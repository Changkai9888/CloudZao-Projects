// SPDX-License-Identifier: MIT
#include "plus_font.h"
#include "font.h"
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    plus_font::Font font;
    if (!font.Open(plus_font_data, plus_font_data_size)) return 3;
    static uint8_t pixels[152 * 684] = {};
    plus_font::Surface1bpp surface{pixels, sizeof(pixels), 1216, 684, 152, true};
    const char* text = u8"中文清晰 Aa09，稍实笔画。\nNoto Sans SC Plus 32px / 500\n"
        u8"0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n"
        u8"你好，世界！“森林里的朋友” 《记事本》\n￥100.00 ± × ÷ ≤ ≥ ℃\n基线：Agjpqy 0123";
    auto result = plus_font::DrawText(font, surface, text, std::strlen(text),
                                    24, 24 + font.metrics().ascent);
    if (result.status != plus_font::Status::Ok || result.missing_glyphs || result.invalid_utf8) return 4;
    const int y = 500;
    for (int row = y; row < 600; ++row) std::memset(pixels + row * 152, 0xff, 152);
    const char* reverse = u8"黑底白字 Aa09";
    result = plus_font::DrawText(font, surface, reverse, std::strlen(reverse),
                                24, y + 18 + font.metrics().ascent, false);
    if (result.status != plus_font::Status::Ok || result.missing_glyphs) return 5;
    FILE* out = std::fopen(argv[1], "wb");
    if (!out) return 6;
    const bool ok = std::fprintf(out, "P4\n1216 684\n") > 0 &&
        std::fwrite(pixels, 1, sizeof(pixels), out) == sizeof(pixels);
    const int close_status = std::fclose(out);
    if (!ok || close_status != 0) return 7;
    // This call keeps the license linked. A real firmware should expose it
    // from its Help/About page, not just print it to an inaccessible log.
    std::printf("Rendered %u px, weight %u, threshold %u; %u glyphs.\n%s\n",
        unsigned(font.spec().pixels), unsigned(font.spec().weight),
        unsigned(font.spec().threshold), unsigned(font.glyph_count()), plus_font_data_license);
    return 0;
}
