#ifndef EDGE_WEBSOCKET_CLIENT_HPP
#define EDGE_WEBSOCKET_CLIENT_HPP

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace edge
{

struct DetectionBox
{
    std::string id;
    std::string label;
    double confidence = 0.0;
    double x          = 0.0;
    double y          = 0.0;
    double width      = 0.0;
    double height     = 0.0;
};

struct DetectionEvent
{
    std::string device_id        = "RK3588-CAR-01";
    std::string door_state       = "CLOSED";
    std::string behavior         = "STANDING";
    double behavior_confidence   = 0.80;
    int people_count             = 1;
    int dwell_seconds            = 0;
    std::string video_stream_url = "http://127.0.0.1/hls/orangepi/index.m3u8";
    std::vector<DetectionBox> detections;
};

struct EdgeWebSocketConfig
{
    std::string url = "ws://10.41.187.210:8080/ws/edge";
    std::size_t max_queue_size = 32;
    int reconnect_delay_ms = 3000;
};

class EdgeWebSocketClient
{
public:
    explicit EdgeWebSocketClient(EdgeWebSocketConfig config = {});
    ~EdgeWebSocketClient();

    EdgeWebSocketClient(const EdgeWebSocketClient &)            = delete;
    EdgeWebSocketClient &operator=(const EdgeWebSocketClient &) = delete;

    bool start();
    void stop();

    bool enqueue_text(std::string payload);
    bool enqueue_event(const DetectionEvent &event);

    bool is_running() const;
    bool is_connected() const;

    static std::string make_event_json(const DetectionEvent &event);
    static DetectionEvent make_demo_event(std::uint64_t frame_index);

private:
    struct ParsedUrl
    {
        std::string host;
        std::string port;
        std::string path;
    };

    void worker_loop();
    bool connect_server();
    void close_socket();
    bool send_next_message(const std::string &payload);
    void drain_incoming();

    static bool parse_url(const std::string &url, ParsedUrl &parsed);
    static std::string json_escape(const std::string &value);
    static std::string make_websocket_key();
    static std::string base64_encode(const unsigned char *data, std::size_t len);

    EdgeWebSocketConfig config_;
    ParsedUrl parsed_url_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    int socket_fd_ = -1;

    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::deque<std::string> queue_;
    std::thread worker_;
};

} // namespace edge

#endif
