﻿#ifndef BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H
#define BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H

#include "ass_config.h"

namespace config {

typedef struct live_render_config_ : public ass_config_t {
    // ffmpeg setting
    std::string ffmpeg_path_;
    std::string output_file_path_;
    bool post_convert;

    // video setting
    int fps_;
    std::string video_bitrate_;
    std::string audio_bitrate_;

    // stream setting
    std::string filename_;
    std::string stream_address_;

    std::string actual_file_name_; // In some cases, the file name may not legal,
                                   // we will store the actual file name here.
                                   // This may be the same as the original file name,
                                   // or it may be a timestamp format.
                                   // The encoding of this field is always UTF8

    // live info
    uint64_t user_uid_;

    live_render_config_() {
    }
    live_render_config_(const ass_config_t &cfg) : ass_config_t(cfg) {
    }

} live_render_config_t;

live_render_config_t get_user_live_render_config();

} // namespace config

#endif //BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H
