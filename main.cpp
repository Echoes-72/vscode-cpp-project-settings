#include "rknn/rkYolov5s.hpp"
#include "rknn/rknnPool.hpp"
#include "streamer.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace streamer;

using time_point            = std::chrono::high_resolution_clock::time_point;
using high_resolution_clock = std::chrono::high_resolution_clock;

class MovingAverage
{
    int size;
    int pos;
    bool crossed;
    std::vector<double> v;

public:
    explicit MovingAverage(int sz)
    {
        size = sz;
        v.resize(size);
        pos     = 0;
        crossed = false;
    }

    void add_value(double value)
    {
        v[pos] = value;
        pos++;
        if (pos == size)
        {
            pos     = 0;
            crossed = true;
        }
    }

    double get_average()
    {
        double avg = 0.0;
        int last   = crossed ? size : pos;
        int k      = 0;
        for (k = 0; k < last; k++)
        {
            avg += v[k];
        }
        return avg / (double)last;
    }
};

static void add_delay(size_t streamed_frames, size_t fps, double elapsed, double avg_frame_time)
{
    // compute min number of frames that should have been streamed based on fps and elapsed
    double dfps            = fps;
    size_t min_streamed    = (size_t)(dfps * elapsed);
    size_t min_plus_margin = min_streamed + 2;

    if (streamed_frames > min_plus_margin)
    {
        size_t excess  = streamed_frames - min_plus_margin;
        double dexcess = excess;

        // add a delay ~ excess*processing_time
// #define SHOW_DELAY
#ifdef SHOW_DELAY
        double delay = dexcess * avg_frame_time * 1000000.0;
        printf("frame %07lu adding delay %.4f\n", streamed_frames, delay);
        printf("avg fps = %.2f\n", streamed_frames / elapsed);
#endif
        usleep(dexcess * avg_frame_time * 1000000.0);
    }
}

void stream_frame(Streamer &streamer, const cv::Mat &image)
{
    streamer.stream_frame(image.data);
}

void stream_frame(Streamer &streamer, const cv::Mat &image, int64_t frame_duration)
{
    streamer.stream_frame(image.data, frame_duration);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("must provide one command argument with the video file or stream to open\n");
        return 1;
    }

    unsigned int video_Index;
    video_Index = std::stoi(std::string(argv[1]));
    cv::VideoCapture capture;
    capture = cv::VideoCapture(video_Index);

    if (!capture.isOpened())
    {
        fprintf(stderr, "could not open video %u\n", video_Index);
        capture.release();
        return 1;
    }

    int cap_frame_width  = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int cap_frame_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    int cap_fps = capture.get(cv::CAP_PROP_FPS);
    printf("video info w = %d, h = %d, fps = %d\n", cap_frame_width, cap_frame_height, cap_fps);

    int stream_fps = cap_fps;

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

    streamer.enable_av_debug_log();

    streamer.init(streamer_config);

    size_t streamed_frames = 0;

    high_resolution_clock clk;
    time_point time_start = clk.now();
    time_point time_prev  = time_start;

    MovingAverage moving_average(10);
    double avg_frame_time;

    time_point time_stop = clk.now();
    auto elapsed_time    = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_start);
    auto frame_time      = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_prev);

    // 初始化推理线程池
    const int threadNum   = 4;
    const char *modelPath = "model/yolov5s-640-640.rknn";
    rknnPool<rkYolov5s, cv::Mat, cv::Mat> testPool(modelPath, threadNum);
    if (testPool.init() != 0)
    {
        printf("rknnPool init fail!\n");
        return -1;
    }

    // 计算平均帧率
    struct timeval time;
    gettimeofday(&time, nullptr);
    auto startTime = time.tv_sec * 1000 + time.tv_usec / 1000;

    // 开始推理
    uint frames     = 0;
    auto beforeTime = startTime;

    while (capture.isOpened())
    {
        cv::Mat frame;
        if (capture.read(frame) == false)
        {
            break;
        };
        if (testPool.put(frame) != 0)
        {
            break;
        };

        if (frames >= threadNum && testPool.get(frame) != 0)
        {
            break;
        };

        // 推流
        stream_frame(streamer, frame, frame_time.count() * streamer.inv_stream_timebase);
        time_stop    = clk.now();
        elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_start);
        frame_time   = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_prev);
        time_prev    = time_stop;

        frames++;
        if (frames % 120 == 0)
        {
            gettimeofday(&time, nullptr);
            auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;
            printf("120帧内平均帧率:\t %f fps/s\n", 120.0 / float(currentTime - beforeTime) * 1000.0);
            beforeTime = currentTime;
        }
    }
    capture.release();

    return 0;
}
