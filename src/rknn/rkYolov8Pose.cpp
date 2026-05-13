#include "rknn/rkYolov8Pose.hpp"

#include "opencv2/imgproc/imgproc.hpp"
#include "rknn/coreNum.hpp"
#include "rknn/preprocess.h"
#include "rknn/rknn_api.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
constexpr int kPoseKeypointNum = 17;
constexpr int kPoseFeatureNum  = 5 + kPoseKeypointNum * 3;
constexpr int kDflBins         = 16;

struct PosePoint
{
    float x;
    float y;
    float score;
};

struct PoseResult
{
    cv::Rect box;
    float score;
    std::array<PosePoint, kPoseKeypointNum> keypoints;
};

const int kSkeleton[][2] = {
    {0, 1},   {0, 2},   {1, 3},   {2, 4},   {5, 6},   {5, 7},   {7, 9},
    {6, 8},   {8, 10},  {5, 11},  {6, 12},  {11, 12}, {11, 13}, {13, 15},
    {12, 14}, {14, 16},
};

void dump_tensor_attr(rknn_tensor_attr *attr)
{
    std::string shape_str = attr->n_dims < 1 ? "" : std::to_string(attr->dims[0]);
    for (int i = 1; i < attr->n_dims; ++i)
    {
        shape_str += ", " + std::to_string(attr->dims[i]);
    }

    printf(
        "  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
        attr->index,
        attr->name,
        attr->n_dims,
        shape_str.c_str(),
        attr->n_elems,
        attr->size,
        get_format_string(attr->fmt),
        get_type_string(attr->type),
        get_qnt_type_string(attr->qnt_type),
        attr->zp,
        attr->scale
    );
}

unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    if (fp == NULL)
    {
        return NULL;
    }

    if (fseek(fp, ofst, SEEK_SET) != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    unsigned char *data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }

    fread(data, 1, sz, fp);
    return data;
}

unsigned char *load_model(const char *filename, int *model_size)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    unsigned char *data = load_data(fp, 0, size);
    fclose(fp);

    *model_size = size;
    return data;
}

float clamp_float(float val, float min_val, float max_val)
{
    return std::max(min_val, std::min(max_val, val));
}

float calc_iou(const cv::Rect &a, const cv::Rect &b)
{
    int inter_area = (a & b).area();
    int union_area = a.area() + b.area() - inter_area;
    if (union_area <= 0)
    {
        return 0.0f;
    }
    return (float)inter_area / (float)union_area;
}

std::vector<PoseResult> nms_pose(std::vector<PoseResult> &poses, float threshold)
{
    std::sort(poses.begin(), poses.end(), [](const PoseResult &a, const PoseResult &b) { return a.score > b.score; });

    std::vector<PoseResult> picked;
    std::vector<int> removed(poses.size(), 0);
    for (size_t i = 0; i < poses.size(); ++i)
    {
        if (removed[i])
        {
            continue;
        }

        picked.push_back(poses[i]);
        for (size_t j = i + 1; j < poses.size(); ++j)
        {
            if (!removed[j] && calc_iou(poses[i].box, poses[j].box) > threshold)
            {
                removed[j] = 1;
            }
        }
    }

    return picked;
}

bool parse_pose_output_layout(const rknn_tensor_attr &attr, int *feature_num, int *proposal_num, bool *transposed)
{
    if (attr.n_dims < 2)
    {
        return false;
    }

    int d0 = attr.dims[attr.n_dims - 2];
    int d1 = attr.dims[attr.n_dims - 1];

    if (d0 <= 128 && d1 > d0)
    {
        *feature_num  = d0;
        *proposal_num = d1;
        *transposed   = false;
        return true;
    }

    if (d1 <= 128 && d0 > d1)
    {
        *feature_num  = d1;
        *proposal_num = d0;
        *transposed   = true;
        return true;
    }

    return false;
}

float read_output_value(const float *data, int feature_num, int proposal_num, bool transposed, int proposal, int feature)
{
    if (transposed)
    {
        return data[proposal * feature_num + feature];
    }
    return data[feature * proposal_num + proposal];
}

float softmax_integral(const float *data, int channel_stride)
{
    float max_value = data[0];
    for (int i = 1; i < kDflBins; ++i)
    {
        max_value = std::max(max_value, data[i * channel_stride]);
    }

    float sum = 0.0f;
    float weighted_sum = 0.0f;
    for (int i = 0; i < kDflBins; ++i)
    {
        float value = std::exp(data[i * channel_stride] - max_value);
        sum += value;
        weighted_sum += value * (float)i;
    }

    if (sum <= 0.0f)
    {
        return 0.0f;
    }
    return weighted_sum / sum;
}

void process_pose_level(
    const float *box_output,
    const rknn_tensor_attr &box_attr,
    const float *class_output,
    const float *kpt_output,
    const float *visibility_output,
    int model_w,
    int model_h,
    int image_w,
    int image_h,
    float scale_w,
    float scale_h,
    float conf_threshold,
    std::vector<PoseResult> &proposals
)
{
    if (box_attr.n_dims < 4)
    {
        return;
    }

    int grid_h = box_attr.dims[2];
    int grid_w = box_attr.dims[3];
    int grid_len = grid_h * grid_w;
    if (grid_len <= 0)
    {
        return;
    }

    float stride_w = (float)model_w / (float)grid_w;
    float stride_h = (float)model_h / (float)grid_h;

    for (int y = 0; y < grid_h; ++y)
    {
        for (int x = 0; x < grid_w; ++x)
        {
            int pos = y * grid_w + x;
            float score = class_output[pos];
            if (score < conf_threshold)
            {
                continue;
            }

            const float *box_ptr = box_output + pos;
            float left = softmax_integral(box_ptr + 0 * kDflBins * grid_len, grid_len) * stride_w;
            float top = softmax_integral(box_ptr + 1 * kDflBins * grid_len, grid_len) * stride_h;
            float right = softmax_integral(box_ptr + 2 * kDflBins * grid_len, grid_len) * stride_w;
            float bottom = softmax_integral(box_ptr + 3 * kDflBins * grid_len, grid_len) * stride_h;

            float anchor_x = ((float)x + 0.5f) * stride_w;
            float anchor_y = ((float)y + 0.5f) * stride_h;
            float x1 = clamp_float((anchor_x - left) / scale_w, 0.0f, (float)(image_w - 1));
            float y1 = clamp_float((anchor_y - top) / scale_h, 0.0f, (float)(image_h - 1));
            float x2 = clamp_float((anchor_x + right) / scale_w, 0.0f, (float)(image_w - 1));
            float y2 = clamp_float((anchor_y + bottom) / scale_h, 0.0f, (float)(image_h - 1));

            PoseResult pose;
            pose.box = cv::Rect(
                cv::Point((int)x1, (int)y1),
                cv::Point(std::max((int)x1 + 1, (int)x2), std::max((int)y1 + 1, (int)y2))
            );
            pose.score = score;

            for (int k = 0; k < kPoseKeypointNum; ++k)
            {
                pose.keypoints[k].x = clamp_float(kpt_output[(k * 2) * grid_len + pos] / scale_w, 0.0f, (float)(image_w - 1));
                pose.keypoints[k].y = clamp_float(kpt_output[(k * 2 + 1) * grid_len + pos] / scale_h, 0.0f, (float)(image_h - 1));
                pose.keypoints[k].score = visibility_output[k * grid_len + pos];
            }

            proposals.push_back(pose);
        }
    }
}

std::vector<PoseResult> post_process_pose(
    rknn_output *outputs,
    rknn_tensor_attr *attrs,
    uint32_t output_num,
    int model_w,
    int model_h,
    int image_w,
    int image_h,
    float scale_w,
    float scale_h,
    float conf_threshold,
    float nms_threshold
)
{
    if (output_num >= 12)
    {
        std::vector<PoseResult> proposals;
        for (uint32_t i = 0; i + 3 < output_num && i < 12; i += 4)
        {
            process_pose_level(
                (float *)outputs[i].buf,
                attrs[i],
                (float *)outputs[i + 1].buf,
                (float *)outputs[i + 2].buf,
                (float *)outputs[i + 3].buf,
                model_w,
                model_h,
                image_w,
                image_h,
                scale_w,
                scale_h,
                conf_threshold,
                proposals
            );
        }
        return nms_pose(proposals, nms_threshold);
    }

    const float *output = (float *)outputs[0].buf;
    const rknn_tensor_attr &attr = attrs[0];
    int feature_num = 0;
    int proposal_num = 0;
    bool transposed = false;
    if (!parse_pose_output_layout(attr, &feature_num, &proposal_num, &transposed) || feature_num < kPoseFeatureNum)
    {
        printf("Unsupported YOLOv8 pose output shape.\n");
        return {};
    }

    std::vector<PoseResult> proposals;
    for (int i = 0; i < proposal_num; ++i)
    {
        float score = read_output_value(output, feature_num, proposal_num, transposed, i, 4);
        if (score < conf_threshold)
        {
            continue;
        }

        float cx = read_output_value(output, feature_num, proposal_num, transposed, i, 0);
        float cy = read_output_value(output, feature_num, proposal_num, transposed, i, 1);
        float w  = read_output_value(output, feature_num, proposal_num, transposed, i, 2);
        float h  = read_output_value(output, feature_num, proposal_num, transposed, i, 3);

        float x1 = clamp_float((cx - w * 0.5f) / scale_w, 0.0f, (float)(image_w - 1));
        float y1 = clamp_float((cy - h * 0.5f) / scale_h, 0.0f, (float)(image_h - 1));
        float x2 = clamp_float((cx + w * 0.5f) / scale_w, 0.0f, (float)(image_w - 1));
        float y2 = clamp_float((cy + h * 0.5f) / scale_h, 0.0f, (float)(image_h - 1));

        PoseResult pose;
        pose.box = cv::Rect(
            cv::Point((int)x1, (int)y1),
            cv::Point(std::max((int)x1 + 1, (int)x2), std::max((int)y1 + 1, (int)y2))
        );
        pose.score = score;

        for (int k = 0; k < kPoseKeypointNum; ++k)
        {
            int offset = 5 + k * 3;
            pose.keypoints[k].x = clamp_float(
                read_output_value(output, feature_num, proposal_num, transposed, i, offset) / scale_w,
                0.0f,
                (float)(image_w - 1)
            );
            pose.keypoints[k].y = clamp_float(
                read_output_value(output, feature_num, proposal_num, transposed, i, offset + 1) / scale_h,
                0.0f,
                (float)(image_h - 1)
            );
            pose.keypoints[k].score = read_output_value(output, feature_num, proposal_num, transposed, i, offset + 2);
        }

        proposals.push_back(pose);
    }

    return nms_pose(proposals, nms_threshold);
}

void draw_pose(cv::Mat &image, const PoseResult &pose, float keypoint_threshold)
{
    cv::rectangle(image, pose.box, cv::Scalar(0, 128, 255), 2);

    char text[64];
    snprintf(text, sizeof(text), "person %.1f%%", pose.score * 100.0f);
    cv::putText(
        image,
        text,
        cv::Point(pose.box.x, std::max(0, pose.box.y - 8)),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        cv::Scalar(0, 128, 255),
        2
    );

    for (const auto &link : kSkeleton)
    {
        const PosePoint &p1 = pose.keypoints[link[0]];
        const PosePoint &p2 = pose.keypoints[link[1]];
        if (p1.score >= keypoint_threshold && p2.score >= keypoint_threshold)
        {
            cv::line(image, cv::Point((int)p1.x, (int)p1.y), cv::Point((int)p2.x, (int)p2.y), cv::Scalar(0, 255, 0), 2);
        }
    }

    for (const PosePoint &point : pose.keypoints)
    {
        if (point.score >= keypoint_threshold)
        {
            cv::circle(image, cv::Point((int)point.x, (int)point.y), 3, cv::Scalar(0, 0, 255), -1);
        }
    }
}
} // namespace

rkYolov8Pose::rkYolov8Pose(const std::string &model_path)
{
    this->model_path = model_path;
    model_data       = NULL;
    input_attrs      = NULL;
    output_attrs     = NULL;
    ctx              = 0;
    nms_threshold    = 0.45f;
    box_conf_threshold = 0.25f;
    keypoint_threshold = 0.30f;
}

int rkYolov8Pose::init(rknn_context *ctx_in, bool share_weight)
{
    printf("Loading YOLOv8 pose model...\n");
    int model_data_size = 0;
    model_data = load_model(model_path.c_str(), &model_data_size);
    if (model_data == NULL)
    {
        return -1;
    }

    if (share_weight)
    {
        ret = rknn_dup_context(ctx_in, &ctx);
    }
    else
    {
        ret = rknn_init(&ctx, model_data, model_data_size, 0, NULL);
    }

    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        return -1;
    }

    rknn_core_mask core_mask = RKNN_NPU_CORE_0;
    switch (get_core_num())
    {
    case 0: core_mask = RKNN_NPU_CORE_0; break;
    case 1: core_mask = RKNN_NPU_CORE_1; break;
    case 2: core_mask = RKNN_NPU_CORE_2; break;
    default: break;
    }

    ret = rknn_set_core_mask(ctx, core_mask);
    if (ret < 0)
    {
        printf("rknn_set_core_mask error ret=%d\n", ret);
        return -1;
    }

    rknn_sdk_version version;
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version, sizeof(version));
    if (ret < 0)
    {
        printf("rknn_query version error ret=%d\n", ret);
        return -1;
    }
    printf("sdk version: %s driver version: %s\n", version.api_version, version.drv_version);

    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        printf("rknn_query io num error ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    if (io_num.n_output < 1)
    {
        printf("YOLOv8 pose model needs at least one output.\n");
        return -1;
    }

    input_attrs = (rknn_tensor_attr *)calloc(io_num.n_input, sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0)
        {
            printf("rknn_query input attr error ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    output_attrs = (rknn_tensor_attr *)calloc(io_num.n_output, sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0)
        {
            printf("rknn_query output attr error ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        channel = input_attrs[0].dims[1];
        height  = input_attrs[0].dims[2];
        width   = input_attrs[0].dims[3];
    }
    else
    {
        height  = input_attrs[0].dims[1];
        width   = input_attrs[0].dims[2];
        channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n", height, width, channel);

    memset(inputs, 0, sizeof(inputs));
    inputs[0].index        = 0;
    inputs[0].type         = RKNN_TENSOR_UINT8;
    inputs[0].size         = width * height * channel;
    inputs[0].fmt          = RKNN_TENSOR_NHWC;
    inputs[0].pass_through = 0;

    return 0;
}

rknn_context *rkYolov8Pose::get_pctx()
{
    return &ctx;
}

cv::Mat rkYolov8Pose::infer(cv::Mat &orig_img)
{
    std::lock_guard<std::mutex> lock(mtx);

    cv::Mat rgb_img;
    cv::cvtColor(orig_img, rgb_img, cv::COLOR_BGR2RGB);

    cv::Size target_size(width, height);
    cv::Mat resized_img(target_size.height, target_size.width, CV_8UC3);
    float scale_w = (float)target_size.width / (float)rgb_img.cols;
    float scale_h = (float)target_size.height / (float)rgb_img.rows;

    if (rgb_img.cols != width || rgb_img.rows != height)
    {
        rga_buffer_t src;
        rga_buffer_t dst;
        memset(&src, 0, sizeof(src));
        memset(&dst, 0, sizeof(dst));
        ret = resize_rga(src, dst, rgb_img, resized_img, target_size);
        if (ret != 0)
        {
            fprintf(stderr, "resize with rga error\n");
            cv::resize(rgb_img, resized_img, target_size);
        }
        inputs[0].buf = resized_img.data;
    }
    else
    {
        inputs[0].buf = rgb_img.data;
    }

    ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_inputs_set error ret=%d\n", ret);
        return orig_img;
    }

    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (uint32_t i = 0; i < io_num.n_output; i++)
    {
        outputs[i].want_float = 1;
    }

    ret = rknn_run(ctx, NULL);
    if (ret < 0)
    {
        printf("rknn_run error ret=%d\n", ret);
        return orig_img;
    }

    ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get error ret=%d\n", ret);
        return orig_img;
    }

    std::vector<PoseResult> poses = post_process_pose(
        outputs,
        output_attrs,
        io_num.n_output,
        width,
        height,
        orig_img.cols,
        orig_img.rows,
        scale_w,
        scale_h,
        box_conf_threshold,
        nms_threshold
    );

    for (const PoseResult &pose : poses)
    {
        draw_pose(orig_img, pose, keypoint_threshold);
    }

    ret = rknn_outputs_release(ctx, io_num.n_output, outputs);
    return orig_img;
}

rkYolov8Pose::~rkYolov8Pose()
{
    if (ctx)
    {
        rknn_destroy(ctx);
    }

    if (model_data)
    {
        free(model_data);
    }
    if (input_attrs)
    {
        free(input_attrs);
    }
    if (output_attrs)
    {
        free(output_attrs);
    }
}
