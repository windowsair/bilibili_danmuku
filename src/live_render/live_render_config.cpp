﻿#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <filesystem>
#include <vector>

#include "live_render_config.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/fmt/include/fmt/os.h"

#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/filereadstream.h"
#include "thirdparty/rapidjson/schema.h"

#include "config_live_render_template.hpp"

namespace config {

constexpr auto user_live_render_config_path = "live_render_config.json";

inline live_render_config_t get_default_live_render_config() {
    live_render_config_t config{get_default_ass_config()};

    config.ffmpeg_path_ = "tool/";
    config.output_file_path_ = "video/";
    config.post_convert = false;
    config.ffmpeg_thread_queue_size_ = 20000;
    config.render_thread_queue_size_ = 64;


    config.fps_ = 60;
    config.video_bitrate_ = "15M";
    config.audio_bitrate_ = "320K";

    return config;
}

inline void generate_default_live_render_config() {

    auto out = fmt::output_file(user_live_render_config_path);
    out.print("{}", config_live_render_template_json);

    out.flush();
    out.close();
}

live_render_config_t get_user_live_render_config() {
    std::filesystem::path file_path(user_live_render_config_path);

    if (!std::filesystem::exists(file_path)) {
        // not exist, then generate
        generate_default_live_render_config();
        return get_default_live_render_config();
    }

    // get user config
    using namespace rapidjson;

    std::vector<char> buffer(65536);

    FILE *fp = fopen(user_live_render_config_path, "rb");
    FileReadStream is(fp, buffer.data(), buffer.size());

    Document doc;
    doc.ParseStream(is);

    // Schema verify
    Document origin_schema_doc;
    if (origin_schema_doc.Parse(config_live_render_template_schema).HasParseError()) {
        // should not be happen...
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "内部错误：Schema无效\n");
        fclose(fp);
        return get_default_live_render_config();
    }
    SchemaDocument schema_doc(origin_schema_doc);
    SchemaValidator validator(schema_doc);

    if (!doc.Accept(validator)) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "配置文件无效。请检查\n");
        fclose(fp);
        exit(0);
    }

    // Accept!
    live_render_config_t config;

    config.ffmpeg_path_  = doc["ffmpeg_path"].GetString();
    config.output_file_path_ = doc["output_path"].GetString();
    config.video_bitrate_ = doc["video_bitrate"].GetString();
    config.audio_bitrate_ = doc["audio_bitrate"].GetString();
    config.encoder_ = doc["encoder"].GetString();
    config.decoder_ = doc["decoder"].GetString();

    for (auto& item: doc["extra_encoder_info"].GetArray()) {
        config.extra_encoder_info_.push_back(item.GetString());
    }
    if (config.extra_encoder_info_.size() == 1 &&
        config.extra_encoder_info_[0].size() == 0) {
        config.extra_encoder_info_.clear();
    }

    config.segment_time_ = doc["segment_time"].GetInt64();
    config.ffmpeg_thread_queue_size_ = doc["ffmpeg_thread_queue_size"].GetInt();
    config.render_thread_queue_size_ = doc["render_thread_queue_size"].GetInt();

    config.font_family_ = doc["font_family"].GetString();
    config.font_color_ = 0xFFFFFF; // white
    config.font_size_ = 25;
    config.font_scale_ = doc["font_scale"].GetFloat();
    config.font_alpha_ = doc["font_alpha"].GetFloat();
    config.font_bold_ = doc["font_bold"].GetBool();
    config.font_outline_ = doc["font_outline"].GetFloat();
    config.font_shadow_ = doc["font_shadow"].GetFloat();
    config.danmaku_show_range_ = doc["danmaku_show_range"].GetFloat();
    config.danmaku_move_time_ = doc["danmaku_move_time"].GetInt();
    config.danmaku_pos_time_ = doc["danmaku_pos_time"].GetInt();

    config.verbose_ = doc["verbose"].GetInt();

    if (doc.HasMember("video_width")) {
        config.video_width_ = doc["video_width"].GetInt();
    }
    if (doc.HasMember("video_height")) {
        config.video_height_ = doc["video_height"].GetInt();
    }
    if (doc.HasMember("fps")) {
        config.fps_ = doc["fps"].GetInt();
    }

    fclose(fp);
    return config;
}
} // namespace config
