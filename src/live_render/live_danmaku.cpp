﻿#include "live_danmaku.h"

#include <vector>

uint8_t data_in[] = {
    0x78, 0xDA, 0xBC, 0x92, 0xC1, 0x8A, 0xD5, 0x30, 0x14, 0x86, 0xCB, 0x80, 0x7B, 0x9F,
    0xE1, 0xAC, 0xB3, 0x48, 0x7B, 0xD3, 0x34, 0xC9, 0x4E, 0xD4, 0x85, 0x1B, 0xC5, 0x61,
    0x60, 0x16, 0x22, 0x21, 0xD3, 0x64, 0x3A, 0x81, 0x36, 0xB9, 0x24, 0xB9, 0x82, 0x0C,
    0x77, 0x7D, 0x1D, 0xD4, 0x85, 0x2F, 0x20, 0x08, 0x3E, 0x80, 0x08, 0x32, 0xE0, 0xC2,
    0xA7, 0xF1, 0xCA, 0xF8, 0x16, 0x92, 0x4E, 0x8B, 0x3A, 0xDE, 0x01, 0x17, 0x8E, 0x5D,
    0x94, 0xF6, 0xFC, 0x49, 0xCE, 0xF9, 0xFF, 0x7C, 0x45, 0xB1, 0xD7, 0x17, 0xB7, 0x8B,
    0xFC, 0xDC, 0xCA, 0xAF, 0x53, 0x68, 0x07, 0x0D, 0x02, 0x1E, 0x3C, 0x3C, 0xB8, 0xBF,
    0x7F, 0xE7, 0xEE, 0x81, 0x3C, 0x7C, 0xB4, 0x7F, 0x0F, 0x10, 0x68, 0x95, 0x14, 0x88,
    0x53, 0x68, 0xBD, 0x4B, 0xC1, 0x1E, 0xAD, 0x92, 0xF5, 0x2E, 0xFF, 0x77, 0x41, 0x69,
    0x03, 0x02, 0xAF, 0x11, 0xE8, 0x21, 0xB6, 0x3E, 0x18, 0x10, 0x25, 0x43, 0x70, 0xAC,
    0x5C, 0x94, 0x83, 0xD1, 0xAA, 0xCF, 0xAB, 0x94, 0x6B, 0x4F, 0x7C, 0x90, 0xC1, 0xFB,
    0xC1, 0x6A, 0x10, 0x35, 0xA9, 0x31, 0x65, 0x08, 0xBA, 0x95, 0x0A, 0x5A, 0xF6, 0xE6,
    0x99, 0xE9, 0x41, 0x60, 0x04, 0xB6, 0xF5, 0x4E, 0xE6, 0x05, 0xF9, 0x3B, 0xCA, 0xDE,
    0x76, 0x27, 0xC9, 0x68, 0x10, 0x25, 0x82, 0xF1, 0x28, 0xD9, 0xFA, 0xDE, 0x07, 0x10,
    0x65, 0x45, 0x1A, 0x86, 0x19, 0xFD, 0xAD, 0x2C, 0x8F, 0x7C, 0xD0, 0xE6, 0x5A, 0xD5,
    0x38, 0x7D, 0x9D, 0x14, 0x93, 0x0A, 0xE9, 0x4F, 0x71, 0x9A, 0xAB, 0xAC, 0xE7, 0x82,
    0x53, 0x83, 0x01, 0x01, 0xDB, 0x2F, 0xE7, 0x5F, 0x3F, 0xBF, 0xB9, 0x38, 0xFB, 0x08,
    0x08, 0x26, 0xC7, 0x94, 0x97, 0x15, 0x47, 0x10, 0x97, 0xA6, 0xB5, 0xD9, 0x31, 0x00,
    0x82, 0xA4, 0x42, 0x67, 0xD2, 0x68, 0x87, 0x35, 0x0B, 0x4E, 0x9A, 0x66, 0x8D, 0xC0,
    0x6A, 0xE3, 0x92, 0x4D, 0xD6, 0x44, 0x10, 0x4F, 0x16, 0xA8, 0x7C, 0x3A, 0x1A, 0x8D,
    0xCB, 0x60, 0xD4, 0xA5, 0xED, 0x21, 0x76, 0x32, 0x3D, 0x5F, 0x9A, 0xD1, 0xF4, 0xD5,
    0xBC, 0xE6, 0x7C, 0x29, 0x61, 0x8C, 0x31, 0x4E, 0x19, 0xC5, 0x8B, 0xDC, 0x35, 0xEF,
    0x96, 0xDA, 0xC4, 0xF6, 0xB2, 0xF3, 0x54, 0xB0, 0xEE, 0xD8, 0xCF, 0xA3, 0xD8, 0x5E,
    0xE6, 0x70, 0xC7, 0x16, 0xC9, 0x0E, 0x26, 0x26, 0x35, 0x2C, 0xA7, 0x93, 0x30, 0x67,
    0x0B, 0x8E, 0x20, 0x05, 0xDB, 0x75, 0x26, 0xC8, 0x2C, 0xFF, 0xA2, 0x30, 0x42, 0x2B,
    0x56, 0x57, 0x18, 0x63, 0x04, 0xAB, 0x3C, 0x4C, 0xC3, 0x6A, 0xD6, 0x54, 0x0D, 0x82,
    0xD5, 0x9C, 0xC7, 0xEB, 0x77, 0xDF, 0x5E, 0x9E, 0x5D, 0x7C, 0x7A, 0xFB, 0x7D, 0xF3,
    0x6A, 0xFB, 0xE2, 0x7C, 0xBB, 0x79, 0xBF, 0xDD, 0x7C, 0x80, 0x49, 0x9F, 0xAF, 0x0C,
    0x60, 0xBD, 0x2E, 0x8A, 0xBD, 0xC3, 0x7F, 0xCE, 0x1A, 0xBE, 0x61, 0xD6, 0x68, 0x59,
    0x51, 0xC2, 0xC9, 0x6E, 0xD4, 0x76, 0x8A, 0x23, 0x69, 0x3B, 0x95, 0x09, 0xB4, 0x2B,
    0xDA, 0x34, 0x13, 0xFB, 0x0B, 0xCC, 0xA6, 0xFB, 0xFE, 0x7F, 0x94, 0x55, 0x94, 0x90,
    0xAA, 0x5E, 0xB0, 0x9B, 0xA7, 0x8C, 0x50, 0xC6, 0xC9, 0x4F, 0xCA, 0xCA, 0xA6, 0xE2,
    0x0D, 0xCF, 0x3B, 0x66, 0xCC, 0x0E, 0x8C, 0xEB, 0x1E, 0xEF, 0xC2, 0xEA, 0x47, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xCC, 0x38, 0x7F, 0x67};




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
