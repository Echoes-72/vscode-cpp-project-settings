#include "rknn/rkYolov5s.hpp"
#include "rknn/rknnPool.hpp"
#include "streamer.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace streamer;

int main(int argc, char *argv[])
{

    cv::VideoCapture capture;

    // 视频文件的输入路径
    if (argc == 2)
    {
        capture.open(argv[1]);
    }
    else
    {
        unsigned int video_Index = 20;
        try
        {
            // video_Index = std::stoi(std::string(argv[1]));
            capture.open(video_Index);
        }
        catch (...)
        {
            fprintf(stderr, "invalid camera index: %s\n", argv[1]);
            return 1;
        }
    }

    if (!capture.isOpened())
    {
        fprintf(stderr, "could not open video %s\n", argv[1]);
        capture.release();
        return 1;
    }

    int cap_frame_width  = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int cap_frame_height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    int cap_fps          = static_cast<int>(capture.get(cv::CAP_PROP_FPS));

    // 很多摄像头通过 OpenCV 获取 FPS 时可能返回 0 或异常值
    if (cap_fps <= 0 || cap_fps > 120)
    {
        cap_fps = 25;
    }

    int stream_fps = 15;

    printf("video info w = %d, h = %d, fps = %d\n", cap_frame_width, cap_frame_height, cap_fps);

    int bitrate = 500000;

    Streamer streamer;

    StreamerConfig streamer_config(
        cap_frame_width,
        cap_frame_height,
        640,
        480,
        stream_fps,
        bitrate,
        "main",
        "rtmp://10.41.187.210:1935/hls/orangepi"
    );

    // streamer.enable_av_debug_log();

    if (streamer.init(streamer_config) != 0)
    {
        fprintf(stderr, "streamer init failed\n");
        capture.release();
        return 1;
    }

    // 固定每帧 duration，避免 DTS/PTS 抖动
    // 假设 streamer.inv_stream_timebase 是 time_base 的倒数
    int64_t frame_duration = streamer.inv_stream_timebase / stream_fps;
    printf("frame_duration: %ld\ninv_stream_timebase: %f\n", frame_duration, streamer.inv_stream_timebase);

    if (frame_duration <= 0)
    {
        fprintf(
            stderr,
            "invalid frame_duration: %ld, inv_stream_timebase: %f, stream_fps: %d\n",
            frame_duration,
            streamer.inv_stream_timebase,
            stream_fps
        );
        capture.release();
        return 1;
    }

    // 初始化 RKNN 推理线程池
    const int threadNum   = 1;
    const char *modelPath = "model/yolov8n-pose.rknn";

    rknnPool<rkYolov5s, cv::Mat, cv::Mat> testPool(modelPath, threadNum);

    if (testPool.init() != 0)
    {
        printf("rknnPool init fail!\n");
        capture.release();
        return -1;
    }

    // FPS 统计
    struct timeval time;
    gettimeofday(&time, nullptr);

    auto startTime  = time.tv_sec * 1000 + time.tv_usec / 1000;
    auto beforeTime = startTime;

    uint frames = 0;

    while (capture.isOpened())
    {
        cv::Mat frame;

        if (!capture.read(frame))
        {
            break;
        }

        if (frame.empty())
        {
            continue;
        }

        if (testPool.put(frame) != 0)
        {
            fprintf(stderr, "testPool put failed\n");
            break;
        }

        // 前 threadNum 帧只用于填满推理流水线
        // 避免前几帧没有推理结果却直接推流
        if (frames < threadNum)
        {
            frames++;
            continue;
        }

        if (testPool.get(frame) != 0)
        {
            fprintf(stderr, "testPool get failed\n");
            break;
        }

        if (frame.empty())
        {
            continue;
        }

        // 使用固定 duration 推流
        streamer.stream_frame(frame.data, frame_duration);

        frames++;

        if (frames % 120 == 0)
        {
            gettimeofday(&time, nullptr);
            auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;

            printf("Fps: %f/s\n", 120.0 / float(currentTime - beforeTime) * 1000.0);

            beforeTime = currentTime;
        }
    }

    // 清空推理线程池中剩余的结果
    while (true)
    {
        cv::Mat frame;

        if (testPool.get(frame) != 0)
        {
            break;
        }

        if (frame.empty())
        {
            continue;
        }

        streamer.stream_frame(frame.data, frame_duration);

        frames++;
    }

    gettimeofday(&time, nullptr);
    auto endTime = time.tv_sec * 1000 + time.tv_usec / 1000;

    printf("Average:\t %f fps/s\n", float(frames) / float(endTime - startTime) * 1000.0);

    capture.release();

    return 0;
}