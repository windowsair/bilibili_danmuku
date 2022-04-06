﻿#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"


live_detail_t live_danmaku::get_room_detail(int live_id) {
    using namespace ix;
    using namespace rapidjson;

    live_detail_t live_detail;

    HttpClient httpClient;
    HttpRequestArgsPtr args = httpClient.createRequest();

    // Timeout options
    args->connectTimeout = 10;
    args->transferTimeout = 10;

    // Redirect options
    args->followRedirects = false;
    args->maxRedirects = 0;

    // Misc
    args->compress = false;
    args->verbose = false;
    args->logger = [](const std::string &msg) { std::cout << msg; };

    // Sync req
    HttpResponsePtr res;
    std::string url =
        std::string("https://api.live.bilibili.com/room/v1/Room/room_init?id=") +
        std::to_string(live_id);

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取房间号失败：{}\n",
                   errorMsg);
        std::abort();
    }

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取房间号失败：{}\n",
                   body);
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return live_detail;
    }

    live_detail.code_ = doc["code"].GetInt();
    if (live_detail.code_ != 0) {
        error_output();
        return live_detail;
    }

    auto &data = doc["data"];
    if (!data.HasMember("room_id")) {
        error_output();
        return live_detail;
    }

    uint64_t room_id = data["room_id"].GetInt64();
    live_detail.room_id_ = room_id;


    live_detail.live_status_ = data["live_status"].GetInt();
    if (live_detail.live_status_ != live_detail::live_status_enum::VALID) {
        return live_detail;
    }


    // get random uid
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<uint64_t> uniform_dist(1, 2e14);
    uint64_t random_uid = uniform_dist(e1) + 1e14;

    live_detail.room_detail_str_ =
        fmt::format(R"({{"roomid":{},"uid":{},"protover":2}})", room_id,
                    random_uid); // {"roomid":0000,"uid":0000,"protover":2}

    return live_detail;
}

/**
 *
 * @param room_id
 * @param qn stream quality
 * @return
 */
std::vector<std::string> live_danmaku::get_live_room_stream(int room_id, int qn) {
    using namespace ix;
    using namespace rapidjson;

    std::vector<std::string> ret;

    live_detail_t live_detail;

    HttpClient httpClient;
    HttpRequestArgsPtr args = httpClient.createRequest();

    // Timeout options
    args->connectTimeout = 10;
    args->transferTimeout = 10;

    // Redirect options
    args->followRedirects = false;
    args->maxRedirects = 0;

    // Misc
    args->compress = false;
    args->verbose = false;
    args->logger = [](const std::string &msg) { std::cout << msg; };

    // Sync req
    HttpResponsePtr res;
    std::string url = fmt::format(
        "https://api.live.bilibili.com/xlive/web-room/v2/index/getRoomPlayInfo"
        "?platform=h5&ptype=8&qn={}&protocol=0,1&format=0,1,2&codec=0,1&room_id={}",
        qn, room_id);

    // qn 20000 -> 4K

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    //printf("%s", body.c_str());

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取直播流信息失败：{}\n", errorMsg);
        return ret;
    }

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取直播流信息失败：{}\n", body);
        return ret;
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return ret;
    }

    live_detail.code_ = doc["code"].GetInt();
    if (live_detail.code_ != 0) {
        error_output();
        return ret;
    }

    auto &data = doc["data"];
    if (data["live_status"] != live_detail_t::VALID) {
        return ret;
    }

    auto &stream_list = data["playurl_info"]["playurl"]["stream"];

    // get max accept qn
    int max_qn = qn;
    int current_qn = qn;
    for (auto &item : stream_list.GetArray()) {
        for (auto &accpet_qn : item["format"][0]["codec"][0]["accept_qn"].GetArray()) {
            int qn_now = accpet_qn.GetInt();
            if (qn_now > max_qn) {
                max_qn = qn_now;
            }
        }
    }

    // try to get max quality stream
    if (max_qn > current_qn) {
        return get_live_room_stream(room_id, max_qn);
    }

    auto get_stream_address_list = [&ret](auto &item) {

        auto base_url = item["format"][0]["codec"][0]["base_url"].GetString();
        auto &url_list = item["format"][0]["codec"][0]["url_info"];
        for (auto &url_info : url_list.GetArray()) {
            auto host = url_info["host"].GetString();
            auto extra = url_info["extra"].GetString();

            ret.emplace_back(fmt::format("{}{}{}", host, base_url, extra));
        }

    };

    // prefer to use flv

    // get flv stream
    for (auto &item : stream_list.GetArray()) {
        std::string format_name(item["format"][0]["format_name"].GetString());
        if (format_name == "flv") {
            get_stream_address_list(item);
        }
    }

    // get ts stream
    for (auto &item : stream_list.GetArray()) {
        std::string format_name(item["format"][0]["format_name"].GetString());
        if (format_name == "ts") {
            get_stream_address_list(item);
        }
    }

    return ret;
}