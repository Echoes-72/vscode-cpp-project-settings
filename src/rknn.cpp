#include "rknn.hpp"
void rknn_process_frame(const cv::Mat &in, cv::Mat &out)
{
    in.copyTo(out);
}