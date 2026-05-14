#include "net/EdgeWebSocketClient.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <netdb.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace edge
{
namespace
{
constexpr int kWebSocketTextOpcode = 0x1;

bool send_all(int fd, const unsigned char *data, std::size_t len)
{
    std::size_t sent = 0;
    while (sent < len)
    {
        ssize_t ret = ::send(fd, data + sent, len - sent, 0);
        if (ret <= 0)
        {
            return false;
        }
        sent += static_cast<std::size_t>(ret);
    }
    return true;
}

bool send_all(int fd, const std::string &data)
{
    return send_all(fd, reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

double clamp(double value, double min_value, double max_value)
{
    return std::max(min_value, std::min(max_value, value));
}

} // namespace

EdgeWebSocketClient::EdgeWebSocketClient(EdgeWebSocketConfig config) : config_(std::move(config))
{
    parse_url(config_.url, parsed_url_);
}

EdgeWebSocketClient::~EdgeWebSocketClient()
{
    stop();
}

bool EdgeWebSocketClient::start()
{
    if (running_.exchange(true))
    {
        return true;
    }

    if (!parse_url(config_.url, parsed_url_))
    {
        running_ = false;
        return false;
    }

    worker_ = std::thread(&EdgeWebSocketClient::worker_loop, this);
    return true;
}

void EdgeWebSocketClient::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    queue_cv_.notify_all();
    if (worker_.joinable())
    {
        worker_.join();
    }
    close_socket();
}

bool EdgeWebSocketClient::enqueue_text(std::string payload)
{
    if (!running_)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (queue_.size() >= config_.max_queue_size)
        {
            queue_.pop_front();
        }
        queue_.push_back(std::move(payload));
    }
    queue_cv_.notify_one();
    return true;
}

bool EdgeWebSocketClient::enqueue_event(const DetectionEvent &event)
{
    return enqueue_text(make_event_json(event));
}

bool EdgeWebSocketClient::is_running() const
{
    return running_;
}

bool EdgeWebSocketClient::is_connected() const
{
    return connected_;
}

void EdgeWebSocketClient::worker_loop()
{
    while (running_)
    {
        if (!connected_ && !connect_server())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
            continue;
        }

        std::string payload;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(500), [this] {
                return !running_ || !queue_.empty();
            });

            if (!running_)
            {
                break;
            }
            if (queue_.empty())
            {
                drain_incoming();
                continue;
            }

            payload = std::move(queue_.front());
            queue_.pop_front();
        }

        if (!send_next_message(payload))
        {
            close_socket();
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
        }
        else
        {
            drain_incoming();
        }
    }
}

bool EdgeWebSocketClient::connect_server()
{
    close_socket();

    addrinfo hints {};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family   = AF_UNSPEC;

    addrinfo *result = nullptr;
    if (::getaddrinfo(parsed_url_.host.c_str(), parsed_url_.port.c_str(), &hints, &result) != 0)
    {
        return false;
    }

    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
        socket_fd_ = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd_ < 0)
        {
            continue;
        }

        if (::connect(socket_fd_, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close_socket();
    }

    ::freeaddrinfo(result);

    if (socket_fd_ < 0)
    {
        return false;
    }

    std::string key = make_websocket_key();
    std::ostringstream request;
    request << "GET " << parsed_url_.path << " HTTP/1.1\r\n"
            << "Host: " << parsed_url_.host << ":" << parsed_url_.port << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << key << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n\r\n";

    if (!send_all(socket_fd_, request.str()))
    {
        close_socket();
        return false;
    }

    char response[1024] {};
    ssize_t received = ::recv(socket_fd_, response, sizeof(response) - 1, 0);
    if (received <= 0)
    {
        close_socket();
        return false;
    }

    std::string text(response, static_cast<std::size_t>(received));
    if (text.find(" 101 ") == std::string::npos)
    {
        close_socket();
        return false;
    }

    connected_ = true;
    return true;
}

void EdgeWebSocketClient::close_socket()
{
    connected_ = false;
    if (socket_fd_ >= 0)
    {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool EdgeWebSocketClient::send_next_message(const std::string &payload)
{
    if (socket_fd_ < 0)
    {
        return false;
    }

    std::vector<unsigned char> frame;
    frame.reserve(payload.size() + 16);
    frame.push_back(0x80 | kWebSocketTextOpcode);

    const std::size_t len = payload.size();
    if (len <= 125)
    {
        frame.push_back(0x80 | static_cast<unsigned char>(len));
    }
    else if (len <= 65535)
    {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xff));
        frame.push_back(static_cast<unsigned char>(len & 0xff));
    }
    else
    {
        frame.push_back(0x80 | 127);
        for (int shift = 56; shift >= 0; shift -= 8)
        {
            frame.push_back(static_cast<unsigned char>((len >> shift) & 0xff));
        }
    }

    unsigned char mask[4] {};
    std::random_device rd;
    for (unsigned char &byte : mask)
    {
        byte = static_cast<unsigned char>(rd());
        frame.push_back(byte);
    }

    for (std::size_t i = 0; i < len; ++i)
    {
        frame.push_back(static_cast<unsigned char>(payload[i]) ^ mask[i % 4]);
    }

    return send_all(socket_fd_, frame.data(), frame.size());
}

void EdgeWebSocketClient::drain_incoming()
{
    if (socket_fd_ < 0)
    {
        return;
    }

    char buffer[512];
    while (::recv(socket_fd_, buffer, sizeof(buffer), MSG_DONTWAIT) > 0)
    {
    }
}

bool EdgeWebSocketClient::parse_url(const std::string &url, ParsedUrl &parsed)
{
    const std::string prefix = "ws://";
    if (url.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    std::string rest = url.substr(prefix.size());
    std::size_t slash_pos = rest.find('/');
    std::string host_port = slash_pos == std::string::npos ? rest : rest.substr(0, slash_pos);

    parsed.path = slash_pos == std::string::npos ? "/" : rest.substr(slash_pos);
    std::size_t colon_pos = host_port.rfind(':');
    if (colon_pos == std::string::npos)
    {
        parsed.host = host_port;
        parsed.port = "80";
    }
    else
    {
        parsed.host = host_port.substr(0, colon_pos);
        parsed.port = host_port.substr(colon_pos + 1);
    }

    return !parsed.host.empty() && !parsed.port.empty() && !parsed.path.empty();
}

std::string EdgeWebSocketClient::make_event_json(const DetectionEvent &event)
{
    std::ostringstream json;
    json << "{\"type\":\"event\",\"payload\":{"
         << "\"deviceId\":\"" << json_escape(event.device_id) << "\","
         << "\"doorState\":\"" << json_escape(event.door_state) << "\","
         << "\"behavior\":\"" << json_escape(event.behavior) << "\","
         << "\"behaviorConfidence\":" << std::fixed << std::setprecision(3) << event.behavior_confidence << ","
         << "\"peopleCount\":" << event.people_count << ","
         << "\"dwellSeconds\":" << event.dwell_seconds << ","
         << "\"videoStreamUrl\":\"" << json_escape(event.video_stream_url) << "\","
         << "\"detections\":[";

    for (std::size_t i = 0; i < event.detections.size(); ++i)
    {
        const DetectionBox &box = event.detections[i];
        if (i != 0)
        {
            json << ",";
        }
        json << "{"
             << "\"id\":\"" << json_escape(box.id) << "\","
             << "\"label\":\"" << json_escape(box.label) << "\","
             << "\"confidence\":" << std::fixed << std::setprecision(3) << box.confidence << ","
             << "\"x\":" << box.x << ","
             << "\"y\":" << box.y << ","
             << "\"width\":" << box.width << ","
             << "\"height\":" << box.height << "}";
    }

    json << "]}}";
    return json.str();
}

DetectionEvent EdgeWebSocketClient::make_demo_event(std::uint64_t frame_index)
{
    DetectionEvent event;
    event.people_count = static_cast<int>((frame_index / 90) % 3) + 1;

    const bool trapped = (frame_index / 240) % 5 == 4;
    if (trapped)
    {
        event.door_state = "CLOSED";
        event.behavior = frame_index % 2 == 0 ? "KNOCKING_DOOR" : "LONG_STAY";
        event.behavior_confidence = 0.82 + 0.08 * std::sin(static_cast<double>(frame_index) / 17.0);
        event.dwell_seconds = 35 + static_cast<int>(frame_index % 35);
    }
    else
    {
        event.door_state = frame_index % 180 < 28 ? "OPEN" : "CLOSED";
        event.behavior = event.door_state == "OPEN" ? "ENTERING" : "STANDING";
        event.behavior_confidence = 0.68 + 0.14 * std::sin(static_cast<double>(frame_index) / 23.0);
        event.dwell_seconds = event.door_state == "CLOSED" ? static_cast<int>(frame_index % 28) : 0;
    }

    for (int i = 0; i < event.people_count; ++i)
    {
        DetectionBox box;
        box.id = "person-" + std::to_string(frame_index) + "-" + std::to_string(i);
        box.label = i == 0 ? event.behavior : "PERSON";
        box.confidence = clamp(event.behavior_confidence - i * 0.04, 0.18, 0.99);
        box.x = clamp(0.12 + i * 0.21 + 0.02 * std::sin(static_cast<double>(frame_index + i) / 13.0), 0.03, 0.78);
        box.y = clamp(0.15 + 0.03 * std::cos(static_cast<double>(frame_index + i) / 19.0), 0.08, 0.34);
        box.width = 0.18;
        box.height = 0.58;
        event.detections.push_back(box);
    }

    return event;
}

std::string EdgeWebSocketClient::json_escape(const std::string &value)
{
    std::ostringstream escaped;
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\': escaped << "\\\\"; break;
        case '"': escaped << "\\\""; break;
        case '\b': escaped << "\\b"; break;
        case '\f': escaped << "\\f"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default: escaped << ch; break;
        }
    }
    return escaped.str();
}

std::string EdgeWebSocketClient::make_websocket_key()
{
    unsigned char bytes[16] {};
    std::random_device rd;
    for (unsigned char &byte : bytes)
    {
        byte = static_cast<unsigned char>(rd());
    }
    return base64_encode(bytes, sizeof(bytes));
}

std::string EdgeWebSocketClient::base64_encode(const unsigned char *data, std::size_t len)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (std::size_t i = 0; i < len; i += 3)
    {
        unsigned int value = data[i] << 16;
        if (i + 1 < len)
        {
            value |= data[i + 1] << 8;
        }
        if (i + 2 < len)
        {
            value |= data[i + 2];
        }

        out.push_back(table[(value >> 18) & 0x3f]);
        out.push_back(table[(value >> 12) & 0x3f]);
        out.push_back(i + 1 < len ? table[(value >> 6) & 0x3f] : '=');
        out.push_back(i + 2 < len ? table[value & 0x3f] : '=');
    }

    return out;
}

} // namespace edge
