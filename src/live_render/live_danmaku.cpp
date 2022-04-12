﻿#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"


size_t live_danmaku::zlib_decompress(void *buffer_in, size_t buffer_in_size) {
    size_t ret;
    auto res = libdeflate_zlib_decompress(this->zlib_handle_, buffer_in, buffer_in_size,
                                          this->zlib_buffer_.data(),
                                          this->zlib_buffer_.size(), &ret);

    if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
        // no enough space, retry
        zlib_buffer_.resize(zlib_buffer_.size() * 2);
        return zlib_decompress(buffer_in, buffer_in_size);
    } else if (res == LIBDEFLATE_BAD_DATA) {
        return 0;
    }

    return ret;
}
void live_danmaku::run(std::string room_info) {

    init_parser();

    ix::WebSocket webSocket;

    std::string url("wss://broadcastlv.chat.bilibili.com/sub");
    webSocket.setUrl(url);

    webSocket.setPingInterval(25);

    webSocket.disablePerMessageDeflate();

    auto packet_len = sizeof(live_danmaku_req_header_t) + room_info.size();
    live_danmaku_req_header_t start_header = {
        .packet_len = htonl(packet_len), .com_1 = htonl(7), .com_2 = htonl(1)};

    start_header.magic[0] = 0x00;
    start_header.magic[1] = 0x10;
    start_header.magic[2] = 0x00;
    start_header.magic[3] = 0x01;

    std::string start_msg_buffer;
    start_msg_buffer.resize(packet_len);
    memcpy(start_msg_buffer.data(), &start_header, sizeof(start_header));
    memcpy(start_msg_buffer.data() + sizeof(start_header), room_info.data(),
           room_info.size());

    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {

            if (msg->binary)
                this->process_websocket_data(msg);

        } else if (msg->type == ix::WebSocketMessageType::Open) {
            webSocket.sendBinary(start_msg_buffer);
            // TODO: output msg
            std::cout << "Connection established" << std::endl;
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            // Maybe SSL is not configured properly
            std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
            std::cout << std::endl << "stop error" << std::endl;
            std::abort();
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            // Maybe SSL is not configured properly
            std::cout << std::endl << "stop close" << std::endl;
            std::abort();
        }
    });

    webSocket.start();

    std::thread([&webSocket]() {
        while (1) {
            using namespace std::chrono_literals;

            const std::string heartbeat{0x00, 0x00, 0x00, 0x1f, 0x00, 0x10, 0x00, 0x01,
                                        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
                                        0x5b, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x20,
                                        0x4f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x5d};

            std::this_thread::sleep_for(25s);
            if (webSocket.getReadyState() == ix::ReadyState::Open) {
                webSocket.sendBinary(heartbeat);
            }
        }
    }).join();
}

void live_danmaku::process_websocket_data(const ix::WebSocketMessagePtr &msg) {
    std::string buffer = msg->str;

    std::vector<std::string> res_list;

    auto add_danmaku_res = [&](char *content, size_t data_len) {
        // hard code. Let's quickly determine the type of content
        const char *cmd = "DANMU";
        constexpr auto cmd_len = 5;

        bool is_danmaku_type = (content + std::min<int>(20, data_len)) !=
                               std::search(content, content + std::min<int>(20, data_len),
                                           cmd, cmd + cmd_len);

        if (is_danmaku_type) {
            std::string item(content, data_len);
            item.push_back('\0'); //TODO:
            res_list.emplace_back(item);
        }
    };

    auto process_decompress_data = [&](char *decompress_data, size_t data_len) {
        while (data_len > sizeof(live_danmaku_res_header_t)) {
            auto header = reinterpret_cast<live_danmaku_res_header_t *>(decompress_data);
            auto packet_len = ntohl(header->packet_len);
            auto header_len = ntohs(header->header_len);

            if (data_len < packet_len) {
                break;
            }

            add_danmaku_res(decompress_data + header_len, packet_len - header_len);

            data_len -= packet_len;
            decompress_data += packet_len;
        }
    };

    // Note that Bilibili does not transfer packets across frames,
    // so each frame is a complete packet.

    // Sometimes incomplete frames are transmitted, which have the payload: "[object Object]".
    // Since their header is not long enough, we will just drop them.
    while (buffer.size() > sizeof(live_danmaku_res_header_t)) {
        auto header = reinterpret_cast<live_danmaku_res_header_t *>(buffer.data());

        auto packet_len = ntohl(header->packet_len);
        auto header_len = ntohs(header->header_len);
        auto version = ntohs(header->version);
        auto op = ntohl(header->op);
        //        auto seq = ntohl(header->seq);

        // now received a part of packet
        if (buffer.size() < packet_len) {
            // TODO: debug
            break;
        }

        if (packet_len == header_len) {
            // no data
            //TODO: test
            std::cout << "no data" << std::endl;
            assert(header_len == sizeof(live_danmaku_res_header_t));
            buffer = buffer.substr(packet_len);
            continue;
        }

        if (op != 5) { // danmaku type
            buffer = buffer.substr(packet_len);
            continue;
        }

        // continue to process
        char *payload_ptr = nullptr;
        size_t payload_len;

        if (version == 2) {
            size_t compress_data_size = packet_len - header_len;
            // zlib compress version
            payload_len = zlib_decompress(buffer.data() + header_len, compress_data_size);

            buffer = buffer.substr(packet_len);
            if (payload_len == 0) {
                printf("wrong payload\n");
                continue;
            }

            payload_ptr = static_cast<char *>(zlib_buffer_.data());
            process_decompress_data(payload_ptr, payload_len);

        } else if (version == 1 || version == 0) {
            // normal version
            payload_ptr = static_cast<char *>(buffer.data() + header_len);
            payload_len = packet_len - header_len;
            buffer = buffer.substr(packet_len);

            add_danmaku_res(payload_ptr, payload_len);

        } else {
            printf("\nunknown version: %d\n", version);
            // just ignore this packet
            buffer = buffer.substr(packet_len);
            continue;
        }
    }

    // parse json string, then send to ffmpeg.
    if (!res_list.empty()) {
        process_danmaku_list(res_list);
    }
}

void live_danmaku::init_parser() {
    const auto content_re_str = R"("content\\":\\"(.*?)\\",\\")";
    parse_helper_.content_re_ = new RE2(content_re_str);
    assert(parse_helper_.content_re_->ok());

    const auto danmaku_type_re_str = R"(dm_type\\":(\d*))";
    parse_helper_.danmaku_type_re_ = new RE2(danmaku_type_re_str);
    assert(parse_helper_.danmaku_type_re_->ok());

    const auto damaku_info_re_str = R"("info":[[][[].*?,(\d),.*?,.*?,(\d*))";
    // [_, danmaku_player_type, font_size, _, timestamp]
    parse_helper_.danmaku_info_re_ = new RE2(damaku_info_re_str);
    assert(parse_helper_.danmaku_info_re_->ok());

    const auto danmaku_color_re_str = R"(\\"color\\":(\d*))";
    parse_helper_.danmaku_color_re_ = new RE2(danmaku_color_re_str);
    assert(parse_helper_.danmaku_color_re_->ok());
}

int count = 0;

// FIXME: trim raw content
// FIXME: vertical type danmaku with \\r, delete it!
void live_danmaku::process_danmaku_list(std::vector<std::string> &raw_danmaku) {
    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    std::string content;
    float start_time;

    std::vector<danmaku::danmaku_item_t> danmaku_list;

    if (base_time_ == 0) {
        // find start base time
        uint64_t min_timestamp = UINT64_MAX;

        for (auto &item : raw_danmaku) {
            RE2::PartialMatch(item, *(parse_helper_.danmaku_info_re_),
                              &danmaku_player_type, &timestamp);

            min_timestamp = std::min<uint64_t>(min_timestamp, timestamp);
        }

        base_time_ = min_timestamp - 10; // 10ms offset
    }

    for (auto &item : raw_danmaku) {
        RE2::PartialMatch(item, *(parse_helper_.content_re_), &content);
        RE2::PartialMatch(item, *(parse_helper_.danmaku_type_re_), &danmaku_origin_type);
        RE2::PartialMatch(item, *(parse_helper_.danmaku_info_re_), &danmaku_player_type,
                          &timestamp);
        RE2::PartialMatch(item, *(parse_helper_.danmaku_color_re_), &color);

        start_time = (float)(timestamp - base_time_) / (float)1000.0f;


        constexpr int font_size = 25;
        danmaku_list.emplace_back(content, start_time, danmaku_player_type, font_size,
                                  color);
    }

    assert(danmaku_queue_ != nullptr);

    count += danmaku_list.size();
    printf("%d\n", count);

    danmaku_queue_->enqueue(danmaku_list);
}

