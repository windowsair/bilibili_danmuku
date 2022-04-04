#include <cmath>

#include "ass_danmaku.h"
#include "config.h"
#include "danmaku.h"
#include "time_table.h"

#include "ass_header.hpp"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/fmt/include/fmt/os.h"

namespace ass {

const int danmaku_layer = 0;
const char *danmaku_name = "Danmu";

std::string rgb2bgr(int rgb) {
    // 0xRRGGBB
    char bgr_str[] = "BBGGRR";
    // index          0 2 4
    uint8_t *p = reinterpret_cast<uint8_t *>(&rgb);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t b = p[0];
    uint8_t g = p[1];
    uint8_t r = p[2];
#else
    uint8_t b = p[2];
    uint8_t g = p[1];
    uint8_t r = p[0];
#endif

    memcpy(&bgr_str[0], hex_table[b], 2);
    memcpy(&bgr_str[2], hex_table[g], 2);
    memcpy(&bgr_str[4], hex_table[r], 2);

    return bgr_str;
    //return fmt::format("{:X}{:X}{:X}", b, g, r);
}

/**
 *  unix timestamp to ass style
 *
 * @param time relative values of unix timestamp
 * @return std::string "Hrs:Mins:Secs.hundredths"
 */
std::string timestamp2ass(int time) {
    char time_str[] = "00:00:00.00";
    //       index :   0  3  6  9

    int hundredths = (time % 1000) / 10;
    time /= 1000;

    int Mins = (int)time / 60;
    int Hrs = Mins / 60;

    Mins %= 60;

    int Secs = (int)time % 60;

    memcpy(&time_str[0], time_table[Hrs], 2);
    memcpy(&time_str[3], time_table[Mins], 2);
    memcpy(&time_str[6], time_table[Secs], 2);
    memcpy(&time_str[9], time_table[hundredths], 2);

    return time_str;
}

/**
 * Seconds to ass style
 * @param time 10.123
 * @return std::string "Hrs:Mins:Secs.hundredths"
 */
//// FIXME: float point accuracy and representation errors
std::string time2ass(float time) {
    char time_str[] = "00:00:00.00";
    //       index :   0  3  6  9

    // 1s -> 100 hundredths
    // 0.123-> 12.3 -> 12
    int Mins = (int)time / 60;
    int Hrs = Mins / 60;

    Mins %= 60;
    int Secs = (int)time % 60;

    float fract, _;
    fract = std::modf(time, &_);

    int tmp = (int)(fract * 1000) % 1000; // 0.123 -> 123

    //FIXME: overflow
    int hundredths = (tmp / 10) + ((tmp % 10) >= 5);

    memcpy(&time_str[0], time_table[Hrs], 2);
    memcpy(&time_str[3], time_table[Mins], 2);
    memcpy(&time_str[6], time_table[Secs], 2);
    memcpy(&time_str[9], time_table[hundredths], 2);

    return time_str;

    //return fmt::format("{:02d}:{:02d}:{:02d}.{:02d}", Hrs, Mins, Secs, hundredths);
}

inline std::string
get_ass_header_impl(const config::ass_config_t &config,
                    std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list) {
   
    using namespace fmt::literals;

    std::string ass_font_color = rgb2bgr(config.font_color_);
    std::string ass_font_alpha =
        fmt::format("{:X}", static_cast<uint8_t>(255 * (1.0f - config.font_alpha_)));
    int ass_font_size = config.font_size_ * config.font_scale_;

    return fmt::format(
        ass_header_format, "title"_a = "hello", "chat_server"_a = config.chat_server_,
        "chat_id"_a = config.chat_id_, "event_count"_a = ass_dialogue_list.size(),
        "play_res_x"_a = config.video_width_, "play_res_y"_a = config.video_height_,
        "name"_a = danmaku_name, "font_name"_a = config.font_family_,
        "font_size"_a = ass_font_size, "font_alpha"_a = ass_font_alpha,
        "font_color"_a = ass_font_color, "font_bold"_a = config.font_bold_ ? -1 : 0);
}

inline std::string get_ass_event_impl(const config::ass_config_t &config,
                                      danmaku::ass_dialogue_t &item) {


    std::string self_effect;
    if (item.font_color_ != config.font_color_) {
        self_effect = fmt::format("\\c&H{}", rgb2bgr(item.font_color_));
    }

    int item_font_size = item.font_size_ * config.font_scale_;

    int start_x = config.video_width_ + item.length_ * item_font_size / 2;
    int end_x = 0 - item.length_ * item_font_size / 2;
    int start_y = item_font_size * (item.dialogue_line_index_ + 1);
    int end_y = start_y;

    return fmt::format(
        ass_dialogue_format, danmaku_layer, time2ass(item.start_time_),
        time2ass(item.start_time_ + config.danmaku_move_time_), danmaku_name,
        item.danmaku_type_ == static_cast<int>(danmaku::danmu_type::MOVE) ? "move"
                                                                          : "pos",
        (float)start_x, (float)start_y, (float)end_x, (float)end_y,
        item.font_color_ != config.font_color_ ? self_effect : "", item.context_);
}


std::string get_ass_header(const config::ass_config_t &config,
                           std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list) {
    return get_ass_header_impl(config, ass_dialogue_list);
}

std::string get_ass_event(const config::ass_config_t &config,
                          danmaku::ass_dialogue_t &item) {
    return get_ass_event_impl(config, item);
}


int ass_render(const std::string &output_file_name, const config::ass_config_t &config,
               std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list) {
    

    auto out = fmt::output_file(output_file_name);
    // header
    out.print("{}", get_ass_header_impl(config, ass_dialogue_list));


    // body event
    for (auto &item : ass_dialogue_list) {
        out.print("{}", get_ass_event_impl(config, item));
    }

    out.flush();
    out.close();

    return 0;
}

} // namespace ass