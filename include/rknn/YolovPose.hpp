#ifndef YolovPose_H
#define YolovPose_H

#include "opencv2/core/core.hpp"
#include "rknn_api.h"

#include <mutex>
#include <string>

class YolovPose
{
private:
    int ret;
    std::mutex mtx;
    std::string model_path;
    unsigned char *model_data;

    rknn_context ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    rknn_input inputs[1];

    int channel, width, height;
    float nms_threshold, box_conf_threshold, keypoint_threshold;

public:
    YolovPose(const std::string &model_path);
    int init(rknn_context *ctx_in, bool share_weight);
    rknn_context *get_pctx();
    cv::Mat infer(cv::Mat &orig_img);
    ~YolovPose();
};

#endif
