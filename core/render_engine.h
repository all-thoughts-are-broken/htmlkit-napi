/*
Copyright (C) 2025 NoneBot

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RENDER_ENGINE_H
#define RENDER_ENGINE_H

#include <optional>
#include <string>
#include <vector>

#include "resource_provider.h"

struct RenderOptions {
    std::string html;
    std::string base_url;
    float dpi = 96.0f;
    float width = 800.0f;
    float height = 600.0f;
    float default_font_size = 12.0f;
    std::string font_name = "sans-serif";
    bool allow_refit = true;
    int image_flag = -1;
    std::string language = "zh";
    std::string culture = "CN";
    bool native_data_scheme = true;
    bool debug = false;
};

struct RenderResult {
    std::vector<unsigned char> image;
    std::optional<std::string> debug_html;
};

RenderResult render_html(const RenderOptions& options,
                         ResourceProvider* resource_provider);

#endif // RENDER_ENGINE_H
