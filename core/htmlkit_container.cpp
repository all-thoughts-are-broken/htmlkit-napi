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

#include "htmlkit_container.h"
#include "base64.h"
#include "cairo_wrapper.h"
#include <array>
#include <pango/pango-font.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#pragma region BLANKET_IMPLS

void htmlkit_container::on_anchor_click(const char* url,
                                        const litehtml::element::ptr& el) {}

bool htmlkit_container::on_element_click(const litehtml::element::ptr& el) {
    return true;
}

void htmlkit_container::on_mouse_event(const litehtml::element::ptr& el,
                                       litehtml::mouse_event event) {}

void htmlkit_container::set_caption(const char* caption) {}

void htmlkit_container::set_cursor(const char* cursor) {}

void htmlkit_container::link(const std::shared_ptr<litehtml::document>& doc,
                             const litehtml::element::ptr& el) {}

#pragma endregion

#pragma region CAIRO

litehtml::pixel_t htmlkit_container::pt_to_px(float pt) const {
    return m_info.dpi * pt / 72.0f;
}

litehtml::pixel_t htmlkit_container::get_default_font_size() const {
    return pt_to_px(m_info.default_font_size_pt);
}

void htmlkit_container::draw_list_marker(litehtml::uint_ptr hdc,
                                         const litehtml::list_marker& marker) {
    if (!marker.image.empty()) {
        auto img = get_image(marker.image.c_str(), marker.baseurl);
        if (img) {
            draw_bmp((cairo_t*)hdc, img, marker.pos.x, marker.pos.y,
                     cairo_image_surface_get_width(img),
                     cairo_image_surface_get_height(img));
            cairo_surface_destroy(img);
        }
    } else {
        switch (marker.marker_type) {
        case litehtml::list_style_type_circle: {
            draw_ellipse((cairo_t*)hdc, marker.pos.x, marker.pos.y, marker.pos.width,
                         marker.pos.height, marker.color, 1);
        } break;
        case litehtml::list_style_type_disc: {
            fill_ellipse((cairo_t*)hdc, marker.pos.x, marker.pos.y, marker.pos.width,
                         marker.pos.height, marker.color);
        } break;
        case litehtml::list_style_type_square:
            if (hdc) {
                auto* cr = (cairo_t*)hdc;
                cairo_save(cr);

                cairo_new_path(cr);
                cairo_rectangle(cr, marker.pos.x, marker.pos.y, marker.pos.width,
                                marker.pos.height);

                set_color(cr, marker.color);
                cairo_fill(cr);
                cairo_restore(cr);
            }
            break;
        default:
            /*do nothing*/
            break;
        }
    }
}

void htmlkit_container::get_image_size(const char* src, const char* baseurl,
                                       litehtml::size& sz) {
    auto img = get_image(src, baseurl);
    if (img) {
        sz.width = cairo_image_surface_get_width(img);
        sz.height = cairo_image_surface_get_height(img);
        cairo_surface_destroy(img);
    } else {
        sz.width = 0;
        sz.height = 0;
    }
}

void htmlkit_container::clip_background_layer(cairo_t* cr,
                                              const litehtml::background_layer& layer) {
    rounded_rectangle(cr, layer.border_box, layer.border_radius);
    cairo_clip(cr);

    cairo_rectangle(cr, layer.clip_box.x, layer.clip_box.y, layer.clip_box.width,
                    layer.clip_box.height);
    cairo_clip(cr);
}

void htmlkit_container::draw_image(litehtml::uint_ptr hdc,
                                   const litehtml::background_layer& layer,
                                   const std::string& url,
                                   const std::string& base_url) {
    if (url.empty() || (layer.clip_box.width == 0 && layer.clip_box.height == 0)) {
        return;
    }

    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    clip_background_layer(cr, layer);

    auto bgbmp = get_image(url.c_str(), base_url.c_str());
    if (bgbmp) {
        int image_width = litehtml::round_f(layer.origin_box.width);
        int image_height = litehtml::round_f(layer.origin_box.height);

        if (image_width != cairo_image_surface_get_width(bgbmp) ||
            image_height != cairo_image_surface_get_height(bgbmp)) {
            auto new_img = scale_surface(bgbmp, image_width, image_height);
            cairo_surface_destroy(bgbmp);
            bgbmp = new_img;
        }

        cairo_pattern_t* pattern = cairo_pattern_create_for_surface(bgbmp);
        cairo_matrix_t flib_m;
        cairo_matrix_init_identity(&flib_m);
        cairo_matrix_translate(&flib_m, -layer.origin_box.x, -layer.origin_box.y);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        cairo_pattern_set_matrix(pattern, &flib_m);

        switch (layer.repeat) {
        case litehtml::background_repeat_no_repeat:
            draw_bmp(cr, bgbmp, layer.origin_box.x, layer.origin_box.y,
                     cairo_image_surface_get_width(bgbmp),
                     cairo_image_surface_get_height(bgbmp));
            break;

        case litehtml::background_repeat_repeat_x:
            cairo_set_source(cr, pattern);
            cairo_rectangle(cr, layer.clip_box.left(), layer.origin_box.y,
                            layer.clip_box.width,
                            cairo_image_surface_get_height(bgbmp));
            cairo_fill(cr);
            break;

        case litehtml::background_repeat_repeat_y:
            cairo_set_source(cr, pattern);
            cairo_rectangle(cr, layer.origin_box.x, layer.clip_box.top(),
                            cairo_image_surface_get_width(bgbmp),
                            layer.clip_box.height);
            cairo_fill(cr);
            break;

        case litehtml::background_repeat_repeat:
            cairo_set_source(cr, pattern);
            cairo_rectangle(cr, layer.clip_box.left(), layer.clip_box.top(),
                            layer.clip_box.width, layer.clip_box.height);
            cairo_fill(cr);
            break;
        }

        cairo_pattern_destroy(pattern);
        cairo_surface_destroy(bgbmp);
    }
    cairo_restore(cr);
}

void htmlkit_container::draw_solid_fill(litehtml::uint_ptr hdc,
                                        const litehtml::background_layer& layer,
                                        const litehtml::web_color& color) {
    if (color == litehtml::web_color::transparent) {
        return;
    }

    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    clip_background_layer(cr, layer);

    set_color(cr, color);
    cairo_paint(cr);

    cairo_restore(cr);
}

/**
 * @brief Draw pattern using layer.repeat property.
 *
 * Pattern must be defined relatively to the (layer.origin_box.x, layer.origin_box.y)
 * point. Function calculates rectangles for repeat-x/repeat-y properties and transform
 * pattern to the correct position. Then call draw callback to draw single pattern.
 *
 * @param cr - cairo context
 * @param pattern - cairo pattern
 * @param layer - background layer
 * @param draw - pattern draw function
 */
static void draw_pattern(
    cairo_t* cr, cairo_pattern_t* pattern, const litehtml::background_layer& layer,
    const std::function<void(cairo_t* cr, cairo_pattern_t* pattern, litehtml::pixel_t x,
                             litehtml::pixel_t y, litehtml::pixel_t width,
                             litehtml::pixel_t height)>& draw) {
    litehtml::pixel_t start_x = layer.origin_box.x;
    int num_x = 1;
    litehtml::pixel_t start_y = layer.origin_box.y;
    int num_y = 1;
    if (layer.repeat == litehtml::background_repeat_repeat_x ||
        layer.repeat == litehtml::background_repeat_repeat) {
        if (layer.origin_box.left() > layer.clip_box.left()) {
            int num_left = (int)((layer.origin_box.left() - layer.clip_box.left()) /
                                 layer.origin_box.width);
            if (layer.origin_box.left() - num_left * layer.origin_box.width >
                layer.clip_box.left()) {
                num_left++;
            }
            start_x = layer.origin_box.left() - num_left * layer.origin_box.width;
            num_x += num_left;
        }
        if (layer.origin_box.right() < layer.clip_box.right()) {
            int num_right = (int)((layer.clip_box.right() - layer.origin_box.right()) /
                                  layer.origin_box.width);
            if (layer.origin_box.left() + num_right * layer.origin_box.width <
                layer.clip_box.right()) {
                num_right++;
            }
            num_x += num_right;
        }
    }
    if (layer.repeat == litehtml::background_repeat_repeat_y ||
        layer.repeat == litehtml::background_repeat_repeat) {
        if (layer.origin_box.top() > layer.clip_box.top()) {
            int num_top = (int)((layer.origin_box.top() - layer.clip_box.top()) /
                                layer.origin_box.height);
            if (layer.origin_box.top() - num_top * layer.origin_box.height >
                layer.clip_box.top()) {
                num_top++;
            }
            start_y = layer.origin_box.top() - num_top * layer.origin_box.height;
            num_y += num_top;
        }
        if (layer.origin_box.bottom() < layer.clip_box.bottom()) {
            int num_bottom =
                (int)((layer.clip_box.bottom() - layer.origin_box.bottom()) /
                      layer.origin_box.height);
            if (layer.origin_box.bottom() + num_bottom * layer.origin_box.height <
                layer.clip_box.bottom()) {
                num_bottom++;
            }
            num_y += num_bottom;
        }
    }

    for (int i_x = 0; i_x < num_x; i_x++) {
        for (int i_y = 0; i_y < num_y; i_y++) {
            cairo_matrix_t flib_m;
            cairo_matrix_init_translate(&flib_m,
                                        -(start_x + i_x * layer.origin_box.width),
                                        -(start_y + i_y * layer.origin_box.height));
            cairo_pattern_set_matrix(pattern, &flib_m);
            draw(cr, pattern, start_x + i_x * layer.origin_box.width,
                 start_y + i_y * layer.origin_box.height, layer.origin_box.width,
                 layer.origin_box.height);
        }
    }
}

void htmlkit_container::draw_linear_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::linear_gradient& gradient) {
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    clip_background_layer(cr, layer);

    // Translate pattern to the (layer.origin_box.x, layer.origin_box.y) point
    cairo_pattern_t* pattern =
        cairo_pattern_create_linear(gradient.start.x - (float)layer.origin_box.x,
                                    gradient.start.y - (float)layer.origin_box.y,
                                    gradient.end.x - (float)layer.origin_box.x,
                                    gradient.end.y - (float)layer.origin_box.y);

    for (const auto& color_stop : gradient.color_points) {
        cairo_pattern_add_color_stop_rgba(
            pattern, color_stop.offset, color_stop.color.red / 255.0,
            color_stop.color.green / 255.0, color_stop.color.blue / 255.0,
            color_stop.color.alpha / 255.0);
    }

    draw_pattern(cr, pattern, layer,
                 [](cairo_t* cr, cairo_pattern_t* pattern, litehtml::pixel_t x,
                    litehtml::pixel_t y, litehtml::pixel_t width,
                    litehtml::pixel_t height) {
                     cairo_set_source(cr, pattern);
                     cairo_rectangle(cr, x, y, width, height);
                     cairo_fill(cr);
                 });

    cairo_pattern_destroy(pattern);
    cairo_restore(cr);
}

void htmlkit_container::add_path_arc(cairo_t* cr, double x, double y, double rx,
                                     double ry, double a1, double a2, bool neg) {
    if (rx > 0 && ry > 0) {
        cairo_save(cr);

        cairo_translate(cr, x, y);
        cairo_scale(cr, 1, ry / rx);
        cairo_translate(cr, -x, -y);

        if (neg) {
            cairo_arc_negative(cr, x, y, rx, a1, a2);
        } else {
            cairo_arc(cr, x, y, rx, a1, a2);
        }

        cairo_restore(cr);
    } else {
        cairo_move_to(cr, x, y);
    }
}

void htmlkit_container::draw_borders(litehtml::uint_ptr hdc,
                                     const litehtml::borders& borders,
                                     const litehtml::position& draw_pos,
                                     bool /*root*/) {
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    cairo_new_path(cr);

    litehtml::pixel_t bdr_top = 0;
    litehtml::pixel_t bdr_bottom = 0;
    litehtml::pixel_t bdr_left = 0;
    litehtml::pixel_t bdr_right = 0;

    if (borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden) {
        bdr_top = borders.top.width;
    }
    if (borders.bottom.width != 0 &&
        borders.bottom.style > litehtml::border_style_hidden) {
        bdr_bottom = borders.bottom.width;
    }
    if (borders.left.width != 0 && borders.left.style > litehtml::border_style_hidden) {
        bdr_left = borders.left.width;
    }
    if (borders.right.width != 0 &&
        borders.right.style > litehtml::border_style_hidden) {
        bdr_right = borders.right.width;
    }

    // draw right border
    if (bdr_right != 0) {
        cairo_matrix_t save_matrix;
        cairo_get_matrix(cr, &save_matrix);
        cairo_translate(cr, draw_pos.left(), draw_pos.top());
        cairo_rotate(cr, M_PI);
        cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

        cairo_wrapper::border border(cr, draw_pos.left() - draw_pos.width,
                                     draw_pos.top() - draw_pos.height, draw_pos.top());
        border.side = cairo_wrapper::border::right_side;
        border.color = borders.right.color;
        border.style = borders.right.style;
        border.border_width = bdr_right;
        border.top_border_width = bdr_bottom;
        border.bottom_border_width = bdr_top;
        border.radius_top_x = borders.radius.bottom_right_x;
        border.radius_top_y = borders.radius.bottom_right_y;
        border.radius_bottom_x = borders.radius.top_right_x;
        border.radius_bottom_y = borders.radius.top_right_y;
        border.draw_border();

        cairo_set_matrix(cr, &save_matrix);
    }

    // draw bottom border
    if (bdr_bottom != 0) {
        cairo_matrix_t save_matrix;
        cairo_get_matrix(cr, &save_matrix);
        cairo_translate(cr, draw_pos.left(), draw_pos.top());
        cairo_rotate(cr, -M_PI / 2.0);
        cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

        cairo_wrapper::border border(cr, draw_pos.left() - draw_pos.height,
                                     draw_pos.top(), draw_pos.top() + draw_pos.width);
        border.side = cairo_wrapper::border::bottom_side;
        border.color = borders.bottom.color;
        border.style = borders.bottom.style;
        border.border_width = bdr_bottom;
        border.top_border_width = bdr_left;
        border.bottom_border_width = bdr_right;
        border.radius_top_x = borders.radius.bottom_left_x;
        border.radius_top_y = borders.radius.bottom_left_y;
        border.radius_bottom_x = borders.radius.bottom_right_x;
        border.radius_bottom_y = borders.radius.bottom_right_y;
        border.draw_border();

        cairo_set_matrix(cr, &save_matrix);
    }

    // draw top border
    if (bdr_top != 0) {
        cairo_matrix_t save_matrix;
        cairo_get_matrix(cr, &save_matrix);
        cairo_translate(cr, draw_pos.left(), draw_pos.top());
        cairo_rotate(cr, M_PI / 2.0);
        cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

        cairo_wrapper::border border(cr, draw_pos.left(),
                                     draw_pos.top() - draw_pos.width, draw_pos.top());
        border.side = cairo_wrapper::border::top_side;
        border.color = borders.top.color;
        border.style = borders.top.style;
        border.border_width = bdr_top;
        border.top_border_width = bdr_right;
        border.bottom_border_width = bdr_left;
        border.radius_top_x = borders.radius.top_right_x;
        border.radius_top_y = borders.radius.top_right_y;
        border.radius_bottom_x = borders.radius.top_left_x;
        border.radius_bottom_y = borders.radius.top_left_y;
        border.draw_border();

        cairo_set_matrix(cr, &save_matrix);
    }

    // draw left border
    if (bdr_left != 0) {
        cairo_wrapper::border border(cr, draw_pos.left(), draw_pos.top(),
                                     draw_pos.bottom());
        border.side = cairo_wrapper::border::left_side;
        border.color = borders.left.color;
        border.style = borders.left.style;
        border.border_width = bdr_left;
        border.top_border_width = bdr_top;
        border.bottom_border_width = bdr_bottom;
        border.radius_top_x = borders.radius.top_left_x;
        border.radius_top_y = borders.radius.top_left_y;
        border.radius_bottom_x = borders.radius.bottom_left_x;
        border.radius_bottom_y = borders.radius.bottom_left_y;
        border.draw_border();
    }
    cairo_restore(cr);
}

void htmlkit_container::transform_text(litehtml::string& /*text*/,
                                       litehtml::text_transform /*tt*/) {}

void htmlkit_container::set_clip(const litehtml::position& pos,
                                 const litehtml::border_radiuses& bdr_radius) {
    m_clips.emplace_back(pos, bdr_radius);
}

void htmlkit_container::del_clip() {
    if (!m_clips.empty()) {
        m_clips.pop_back();
    }
}

void htmlkit_container::apply_clip(cairo_t* cr) {
    for (const auto& clip_box : m_clips) {
        rounded_rectangle(cr, clip_box.box, clip_box.radius);
        cairo_clip(cr);
    }
}

void htmlkit_container::draw_ellipse(cairo_t* cr, litehtml::pixel_t x,
                                     litehtml::pixel_t y, litehtml::pixel_t width,
                                     litehtml::pixel_t height,
                                     const litehtml::web_color& color,
                                     litehtml::pixel_t line_width) {
    if (!cr || width == 0 || height == 0)
        return;
    cairo_save(cr);

    apply_clip(cr);

    cairo_new_path(cr);

    cairo_translate(cr, x + width / 2.0, y + height / 2.0);
    cairo_scale(cr, width / 2.0, height / 2.0);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

    set_color(cr, color);
    cairo_set_line_width(cr, line_width);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void htmlkit_container::fill_ellipse(cairo_t* cr, litehtml::pixel_t x,
                                     litehtml::pixel_t y, litehtml::pixel_t width,
                                     litehtml::pixel_t height,
                                     const litehtml::web_color& color) {
    if (!cr || width == 0 || height == 0)
        return;
    cairo_save(cr);

    apply_clip(cr);

    cairo_new_path(cr);

    cairo_translate(cr, x + width / 2.0, y + height / 2.0);
    cairo_scale(cr, width / 2.0, height / 2.0);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);

    set_color(cr, color);
    cairo_fill(cr);

    cairo_restore(cr);
}

const char* htmlkit_container::get_default_font_name() const {
    return m_info.default_font_name.c_str();
}

std::shared_ptr<litehtml::element>
htmlkit_container::create_element(const char* tag_name,
                                  const litehtml::string_map& /*attributes*/,
                                  const std::shared_ptr<litehtml::document>& /*doc*/
) {
    return nullptr;
}

void htmlkit_container::rounded_rectangle(cairo_t* cr, const litehtml::position& pos,
                                          const litehtml::border_radiuses& radius) {
    cairo_new_path(cr);
    if (radius.top_left_x != 0 && radius.top_left_y != 0) {
        add_path_arc(cr, pos.left() + radius.top_left_x, pos.top() + radius.top_left_y,
                     radius.top_left_x, radius.top_left_y, M_PI, M_PI * 3.0 / 2.0,
                     false);
    } else {
        cairo_move_to(cr, pos.left(), pos.top());
    }

    cairo_line_to(cr, pos.right() - radius.top_right_x, pos.top());

    if (radius.top_right_x != 0 && radius.top_right_y != 0) {
        add_path_arc(cr, pos.right() - radius.top_right_x,
                     pos.top() + radius.top_right_y, radius.top_right_x,
                     radius.top_right_y, M_PI * 3.0 / 2.0, 2.0 * M_PI, false);
    }

    cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x);

    if (radius.bottom_right_x != 0 && radius.bottom_right_y != 0) {
        add_path_arc(cr, pos.right() - radius.bottom_right_x,
                     pos.bottom() - radius.bottom_right_y, radius.bottom_right_x,
                     radius.bottom_right_y, 0, M_PI / 2.0, false);
    }

    cairo_line_to(cr, pos.left() - radius.bottom_left_x, pos.bottom());

    if (radius.bottom_left_x != 0 && radius.bottom_left_y != 0) {
        add_path_arc(cr, pos.left() + radius.bottom_left_x,
                     pos.bottom() - radius.bottom_left_y, radius.bottom_left_x,
                     radius.bottom_left_y, M_PI / 2.0, M_PI, false);
    }
}

cairo_surface_t* htmlkit_container::scale_surface(cairo_surface_t* surface, int width,
                                                  int height) {
    int s_width = cairo_image_surface_get_width(surface);
    int s_height = cairo_image_surface_get_height(surface);
    cairo_surface_t* result = cairo_surface_create_similar(
        surface, cairo_surface_get_content(surface), width, height);
    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(surface);
    cairo_t* cr = cairo_create(result);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_BILINEAR);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
    cairo_scale(cr, (double)width / (double)s_width, (double)height / (double)s_height);
    cairo_set_source(cr, pattern);
    cairo_rectangle(cr, 0, 0, s_width, s_height);
    cairo_fill(cr);
    cairo_destroy(cr);
    cairo_pattern_destroy(pattern);
    return result;
}

void htmlkit_container::draw_bmp(cairo_t* cr, cairo_surface_t* bmp, litehtml::pixel_t x,
                                 litehtml::pixel_t y, int cx, int cy) {
    cairo_save(cr);

    {
        cairo_matrix_t flip_m;
        cairo_matrix_init(&flip_m, 1, 0, 0, -1, 0, 0);

        if (cx != cairo_image_surface_get_width(bmp) ||
            cy != cairo_image_surface_get_height(bmp)) {
            auto bmp_scaled = scale_surface(bmp, cx, cy);
            cairo_set_source_surface(cr, bmp_scaled, x, y);
            cairo_paint(cr);
            cairo_surface_destroy(bmp_scaled);
        } else {
            cairo_set_source_surface(cr, bmp, x, y);
            cairo_paint(cr);
        }
    }

    cairo_restore(cr);
}

void htmlkit_container::get_viewport(litehtml::position& viewport) const {
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_info.width;
    viewport.height = m_info.height;
}

void htmlkit_container::get_media_features(litehtml::media_features& media) const {
    media.type = litehtml::media_type_screen;
    media.width = m_info.width;
    media.height = m_info.height;
    media.device_width = m_info.width;
    media.device_height = m_info.height;
    media.color = 8;
    media.monochrome = 0;
    media.color_index = 256;
    media.resolution = m_info.dpi;
}

void htmlkit_container::get_language(litehtml::string& language,
                                     litehtml::string& culture) const {
    language = m_info.language;
    culture = m_info.culture;
}

void htmlkit_container::draw_radial_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::radial_gradient& gradient) {
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    clip_background_layer(cr, layer);

    // Translate pattern to the (layer.origin_box.x, layer.origin_box.y) point
    litehtml::pointF position = gradient.position;
    position.x -= (float)layer.origin_box.x;
    position.y -= (float)layer.origin_box.y;

    cairo_pattern_t* pattern = cairo_pattern_create_radial(
        position.x, position.y, 0, position.x, position.y, gradient.radius.x);

    for (const auto& color_stop : gradient.color_points) {
        cairo_pattern_add_color_stop_rgba(
            pattern, color_stop.offset, color_stop.color.red / 255.0,
            color_stop.color.green / 255.0, color_stop.color.blue / 255.0,
            color_stop.color.alpha / 255.0);
    }

    draw_pattern(cr, pattern, layer,
                 [&gradient, &position](cairo_t* cr, cairo_pattern_t* pattern,
                                        litehtml::pixel_t x, litehtml::pixel_t y,
                                        litehtml::pixel_t w, litehtml::pixel_t h) {
                     cairo_matrix_t save_matrix;
                     cairo_get_matrix(cr, &save_matrix);

                     auto top = (float)y;
                     auto height = (float)h;
                     if (gradient.radius.x != gradient.radius.y) {
                         litehtml::pointF pos = position;
                         pos.x += (float)x;
                         pos.y += (float)y;
                         // Scale height and top of the origin box
                         float aspect_ratio = gradient.radius.x / gradient.radius.y;
                         height *= aspect_ratio;
                         auto center_y = (pos.y - (float)y) * aspect_ratio;
                         top = pos.y - center_y;

                         cairo_translate(cr, pos.x, pos.y);
                         cairo_scale(cr, 1, gradient.radius.y / gradient.radius.x);
                         cairo_translate(cr, -pos.x, -pos.y);
                     }

                     cairo_set_source(cr, pattern);
                     cairo_rectangle(cr, x, top, w, height);
                     cairo_fill(cr);

                     cairo_set_matrix(cr, &save_matrix);
                 });

    cairo_pattern_destroy(pattern);
    cairo_restore(cr);
}

void htmlkit_container::draw_conic_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::conic_gradient& gradient) {
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);
    apply_clip(cr);

    clip_background_layer(cr, layer);

    cairo_pattern_t* pattern = cairo_wrapper::conic_gradient::create_pattern(
        gradient.angle * M_PI / 180.0 - M_PI / 2.0, gradient.radius,
        gradient.color_points);
    if (!pattern)
        return;

    // Translate a pattern to the (layer.origin_box.x, layer.origin_box.y) point
    litehtml::pointF position = gradient.position;
    position.x -= (float)layer.origin_box.x;
    position.y -= (float)layer.origin_box.y;

    draw_pattern(cr, pattern, layer,
                 [&position](cairo_t* cr, cairo_pattern_t* pattern, litehtml::pixel_t x,
                             litehtml::pixel_t y, litehtml::pixel_t w,
                             litehtml::pixel_t h) {
                     cairo_matrix_t flib_m;
                     cairo_matrix_init_translate(&flib_m, -(position.x + (float)x),
                                                 -(position.y + (float)y));
                     cairo_pattern_set_matrix(pattern, &flib_m);

                     cairo_set_source(cr, pattern);
                     cairo_rectangle(cr, x, y, w, h);
                     cairo_fill(cr);
                 });

    cairo_pattern_destroy(pattern);
    cairo_restore(cr);
}

#pragma endregion

#pragma region PANGO

using cairo_font = cairo_wrapper::font_t;

litehtml::uint_ptr
htmlkit_container::create_font(const litehtml::font_description& descr,
                               const litehtml::document* doc,
                               litehtml::font_metrics* fm) {
    litehtml::string_vector tokens;
    litehtml::split_string(descr.family, tokens, ",");
    std::string fonts;

    for (auto& font : tokens) {
        litehtml::trim(font, " \t\r\n\f\v\"\'");
        if (litehtml::value_in_list(font,
                                    "serif;sans-serif;monospace;cursive;fantasy;")) {
            fonts += font + ",";
            continue;
        }
        litehtml::lcase(font);
        if (m_all_fonts.find(font) != m_all_fonts.end()) {
            fonts += font + ",";
        }
    }

    if (fonts.empty()) {
        fonts = "serif,";
    }

    PangoFontDescription* desc = pango_font_description_from_string(fonts.c_str());
    pango_font_description_set_absolute_size(desc, descr.size * PANGO_SCALE);
    if (descr.style == litehtml::font_style_italic) {
        pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
    } else {
        pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);
    }

    pango_font_description_set_weight(desc, (PangoWeight)descr.weight);

    cairo_font* ret = nullptr;

    if (fm) {
        fm->font_size = descr.size;

        cairo_save(m_temp_cr);
        PangoLayout* layout = pango_cairo_create_layout(m_temp_cr);
        PangoContext* context = pango_layout_get_context(layout);
        PangoLanguage* language = pango_language_get_default();
        pango_layout_set_font_description(layout, desc);
        PangoFontMetrics* metrics = pango_context_get_metrics(context, desc, language);

        fm->ascent = PANGO_PIXELS((double)pango_font_metrics_get_ascent(metrics));
        fm->height = PANGO_PIXELS((double)pango_font_metrics_get_height(metrics));
        fm->descent = fm->height - fm->ascent;
        fm->x_height = fm->height;
        fm->draw_spaces =
            (descr.decoration_line != litehtml::text_decoration_line_none);
        fm->sub_shift = descr.size / 5;
        fm->super_shift = descr.size / 3;

        pango_layout_set_text(layout, "x", 1);

        PangoRectangle ink_rect;
        PangoRectangle logical_rect;
        pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
        fm->x_height = ink_rect.height;
        if (fm->x_height == fm->height)
            fm->x_height = fm->x_height * 4 / 5;

        pango_layout_set_text(layout, "0", 1);

        pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
        fm->ch_width = logical_rect.width;

        cairo_restore(m_temp_cr);

        ret = new cairo_font;
        ret->font = desc;
        ret->size = descr.size;
        ret->strikeout =
            (descr.decoration_line & litehtml::text_decoration_line_line_through) != 0;
        ret->underline =
            (descr.decoration_line & litehtml::text_decoration_line_underline) != 0;
        ret->overline =
            (descr.decoration_line & litehtml::text_decoration_line_overline) != 0;
        ret->ascent = fm->ascent;
        ret->descent = fm->descent;
        ret->decoration_color = descr.decoration_color;
        ret->decoration_style = descr.decoration_style;

        auto thinkness = descr.decoration_thickness;
        if (!thinkness.is_predefined()) {
            litehtml::css_length one_em(1.0, litehtml::css_units_em);
            doc->cvt_units(one_em, *fm, 0);
            doc->cvt_units(thinkness, *fm, (int)one_em.val());
        }

        ret->underline_position = -pango_font_metrics_get_underline_position(metrics);
        if (thinkness.is_predefined()) {
            ret->underline_thickness =
                pango_font_metrics_get_underline_thickness(metrics);
        } else {
            ret->underline_thickness = (int)(thinkness.val() * PANGO_SCALE);
        }
        pango_quantize_line_geometry(&ret->underline_thickness,
                                     &ret->underline_position);
        ret->underline_thickness = PANGO_PIXELS(ret->underline_thickness);
        ret->underline_position = PANGO_PIXELS(ret->underline_position);

        ret->strikethrough_position =
            pango_font_metrics_get_strikethrough_position(metrics);
        if (thinkness.is_predefined()) {
            ret->strikethrough_thickness =
                pango_font_metrics_get_strikethrough_thickness(metrics);
        } else {
            ret->strikethrough_thickness = (int)(thinkness.val() * PANGO_SCALE);
        }
        pango_quantize_line_geometry(&ret->strikethrough_thickness,
                                     &ret->strikethrough_position);
        ret->strikethrough_thickness = PANGO_PIXELS(ret->strikethrough_thickness);
        ret->strikethrough_position = PANGO_PIXELS(ret->strikethrough_position);

        ret->overline_position = pango_units_from_double(fm->ascent);
        if (thinkness.is_predefined()) {
            ret->overline_thickness =
                pango_font_metrics_get_underline_thickness(metrics);
        } else {
            ret->overline_thickness = (int)(thinkness.val() * PANGO_SCALE);
        }
        pango_quantize_line_geometry(&ret->overline_thickness, &ret->overline_position);
        ret->overline_thickness = PANGO_PIXELS(ret->overline_thickness);
        ret->overline_position = PANGO_PIXELS(ret->overline_position);

        g_object_unref(layout);
        pango_font_metrics_unref(metrics);
    }

    return (litehtml::uint_ptr)ret;
}

void htmlkit_container::delete_font(litehtml::uint_ptr hFont) {
    auto* fnt = (cairo_font*)hFont;
    if (fnt) {
        pango_font_description_free(fnt->font);
        delete fnt;
    }
}

litehtml::pixel_t htmlkit_container::text_width(const char* text,
                                                litehtml::uint_ptr hFont) {
    auto* fnt = (cairo_font*)hFont;

    cairo_save(m_temp_cr);

    PangoLayout* layout = pango_cairo_create_layout(m_temp_cr);
    pango_layout_set_font_description(layout, fnt->font);

    pango_layout_set_text(layout, text, -1);
    pango_cairo_update_layout(m_temp_cr, layout);

    int x_width, x_height;
    pango_layout_get_pixel_size(layout, &x_width, &x_height);

    cairo_restore(m_temp_cr);

    g_object_unref(layout);

    return (int)x_width;
}

enum class draw_type { DRAW_OVERLINE, DRAW_STRIKETHROUGH, DRAW_UNDERLINE };

static inline void draw_single_line(cairo_t* cr, litehtml::pixel_t x,
                                    litehtml::pixel_t y, litehtml::pixel_t width,
                                    int thickness, draw_type type) {
    double top;
    switch (type) {
    case draw_type::DRAW_UNDERLINE:
        top = y + (double)thickness / 2.0;
        break;
    case draw_type::DRAW_OVERLINE:
        top = y - (double)thickness / 2.0;
        break;
    case draw_type::DRAW_STRIKETHROUGH:
        top = y + 0.5;
        break;
    default:
        top = y;
        break;
    }
    cairo_move_to(cr, x, top);
    cairo_line_to(cr, x + width, top);
}

static void draw_solid_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                            litehtml::pixel_t width, int thickness, draw_type type,
                            litehtml::web_color& color) {
    draw_single_line(cr, x, y, width, thickness, type);

    cairo_set_source_rgba(cr, (double)color.red / 255.0, (double)color.green / 255.0,
                          (double)color.blue / 255.0, (double)color.alpha / 255.0);
    cairo_set_line_width(cr, thickness);
    cairo_stroke(cr);
}

static void draw_dotted_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                             litehtml::pixel_t width, int thickness, draw_type type,
                             litehtml::web_color& color) {
    draw_single_line(cr, x, y, width, thickness, type);

    std::array<double, 2> dashes{0, thickness * 2.0};
    if (thickness == 1)
        dashes[1] += thickness / 2.0;
    cairo_set_line_width(cr, thickness);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_dash(cr, dashes.data(), 2, x);

    cairo_set_source_rgba(cr, (double)color.red / 255.0, (double)color.green / 255.0,
                          (double)color.blue / 255.0, (double)color.alpha / 255.0);
    cairo_stroke(cr);
}

static void draw_dashed_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                             litehtml::pixel_t width, int thickness, draw_type type,
                             litehtml::web_color& color) {
    draw_single_line(cr, x, y, width, thickness, type);

    std::array<double, 2> dashes{thickness * 2.0, thickness * 3.0};
    cairo_set_line_width(cr, thickness);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_dash(cr, dashes.data(), 2, x);

    cairo_set_source_rgba(cr, (double)color.red / 255.0, (double)color.green / 255.0,
                          (double)color.blue / 255.0, (double)color.alpha / 255.0);
    cairo_stroke(cr);
}

static void draw_wavy_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                           litehtml::pixel_t width, int thickness, draw_type type,
                           litehtml::web_color& color) {
    int h_pad = 1;
    int brush_height = (int)thickness * 3 + h_pad * 2;
    int brush_width = brush_height * 2 - 2 * thickness;

    double top;
    switch (type) {
    case draw_type::DRAW_UNDERLINE:
        top = y + (double)brush_height / 2.0;
        break;
    case draw_type::DRAW_OVERLINE:
        top = y - (double)brush_height / 2.0;
        break;
    default:
        top = y;
        break;
    }

    cairo_surface_t* brush_surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, brush_width, brush_height);
    cairo_t* brush_cr = cairo_create(brush_surface);

    cairo_set_source_rgba(brush_cr, (double)color.red / 255.0,
                          (double)color.green / 255.0, (double)color.blue / 255.0,
                          (double)color.alpha / 255.0);
    cairo_set_line_width(brush_cr, thickness);
    double w = thickness / 2.0;
    cairo_move_to(brush_cr, 0, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, w, brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, brush_width / 2.0 - w, (double)thickness / 2.0 + h_pad);
    cairo_line_to(brush_cr, brush_width / 2.0 + w, (double)thickness / 2.0 + h_pad);
    cairo_line_to(brush_cr, brush_width - w,
                  brush_height - (double)thickness / 2.0 - h_pad);
    cairo_line_to(brush_cr, brush_width,
                  brush_height - (double)thickness / 2.0 - h_pad);
    cairo_stroke(brush_cr);
    cairo_destroy(brush_cr);

    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(brush_surface);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_matrix_t patterm_matrix;
    cairo_matrix_init_translate(&patterm_matrix, 0, -top + brush_height / 2.0);
    cairo_pattern_set_matrix(pattern, &patterm_matrix);
    cairo_set_source(cr, pattern);

    cairo_set_line_width(cr, brush_height);
    cairo_move_to(cr, x, top);
    cairo_line_to(cr, x + width, top);
    cairo_stroke(cr);

    cairo_pattern_destroy(pattern);
    cairo_surface_destroy(brush_surface);
}

static void draw_double_line(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                             litehtml::pixel_t width, int thickness, draw_type type,
                             litehtml::web_color& color) {
    cairo_set_line_width(cr, thickness);
    double top1;
    double top2;
    switch (type) {
    case draw_type::DRAW_UNDERLINE:
        top1 = y + (double)thickness / 2.0;
        top2 = top1 + (double)thickness + (double)thickness / 2.0 + 0.5;
        break;
    case draw_type::DRAW_OVERLINE:
        top1 = y - (double)thickness / 2.0;
        top2 = top1 - (double)thickness - (double)thickness / 2.0 - 0.5;
        break;
    case draw_type::DRAW_STRIKETHROUGH:
        top1 = y - (double)thickness + 0.5;
        top2 = y + (double)thickness + 0.5;
        break;
    default:
        top1 = y;
        top2 = y;
        break;
    }
    cairo_move_to(cr, x, top1);
    cairo_line_to(cr, x + width, top1);
    cairo_stroke(cr);
    cairo_move_to(cr, x, top2);
    cairo_line_to(cr, x + width, top2);
    cairo_set_source_rgba(cr, (double)color.red / 255.0, (double)color.green / 255.0,
                          (double)color.blue / 255.0, (double)color.alpha / 255.0);
    cairo_stroke(cr);
}

void htmlkit_container::draw_text(litehtml::uint_ptr hdc, const char* text,
                                  litehtml::uint_ptr hFont, litehtml::web_color color,
                                  const litehtml::position& pos) {
    auto* fnt = (cairo_font*)hFont;
    auto* cr = (cairo_t*)hdc;
    cairo_save(cr);

    apply_clip(cr);

    set_color(cr, color);

    litehtml::web_color decoration_color = color;

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, fnt->font);
    pango_layout_set_text(layout, text, -1);

    if (auto font_options = m_info.font_options) {
        auto ctx = pango_layout_get_context(layout);
        pango_cairo_context_set_font_options(ctx, font_options);
    }

    PangoRectangle ink_rect, logical_rect;
    pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);

    litehtml::pixel_t text_baseline = pos.height - fnt->descent;

    litehtml::pixel_t x = pos.left() + logical_rect.x;
    litehtml::pixel_t y = pos.top();

    cairo_move_to(cr, x, y);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    litehtml::pixel_t tw = 0;

    if (fnt->underline || fnt->strikeout || fnt->overline) {
        tw = text_width(text, hFont);
    }

    if (!fnt->decoration_color.is_current_color) {
        decoration_color = fnt->decoration_color;
    }

    std::array<decltype(&draw_solid_line), litehtml::text_decoration_style_max>
        draw_funcs{
            draw_solid_line,  // text_decoration_style_solid
            draw_double_line, // text_decoration_style_double
            draw_dotted_line, // text_decoration_style_dotted
            draw_dashed_line, // text_decoration_style_dashed
            draw_wavy_line,   // text_decoration_style_wavy
        };

    if (fnt->underline) {
        draw_funcs[fnt->decoration_style](
            cr, x, pos.top() + text_baseline + fnt->underline_position, tw,
            fnt->underline_thickness, draw_type::DRAW_UNDERLINE, decoration_color);
    }

    if (fnt->strikeout) {
        draw_funcs[fnt->decoration_style](
            cr, x, pos.top() + text_baseline - fnt->strikethrough_position, tw,
            fnt->strikethrough_thickness, draw_type::DRAW_STRIKETHROUGH,
            decoration_color);
    }

    if (fnt->overline) {
        draw_funcs[fnt->decoration_style](
            cr, x, pos.top() + text_baseline - fnt->overline_position, tw,
            fnt->overline_thickness, draw_type::DRAW_OVERLINE, decoration_color);
    }

    cairo_restore(cr);

    g_object_unref(layout);
}

#pragma endregion

#pragma region CUSTOM

htmlkit_container::htmlkit_container(const std::string& base_url,
                                     const container_info& info,
                                     ResourceProvider* resource_provider)
    : m_base_url(base_url), m_info(info),
      m_resource_provider(resource_provider) {
    static NullResourceProvider default_provider;
    if (m_resource_provider == nullptr) {
        m_resource_provider = &default_provider;
    }
    m_temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
    m_temp_cr = cairo_create(m_temp_surface);
    cairo_save(m_temp_cr);
    PangoLayout* layout = pango_cairo_create_layout(m_temp_cr);
    PangoContext* context = pango_layout_get_context(layout);
    PangoFontFamily** families;
    int n;
    pango_context_list_families(context, &families, &n);
    for (int i = 0; i < n; i++) {
        PangoFontFamily* family = families[i];
        if (!PANGO_IS_FONT_FAMILY(family))
            continue;
        std::string font_name = pango_font_family_get_name(family);
        litehtml::lcase(font_name);
        m_all_fonts.insert(font_name);
    }
    g_free(families);
    cairo_restore(m_temp_cr);
    g_object_unref(layout);
}

htmlkit_container::~htmlkit_container() {
    cairo_surface_destroy(m_temp_surface);
    cairo_destroy(m_temp_cr);
    for (auto& [_, surface] : m_img_surfaces) {
        cairo_surface_destroy(surface);
    }
}

void htmlkit_container::set_base_url(const char* base_url) {
    if (base_url != nullptr) {
        m_base_url = base_url;
    }
}

std::vector<unsigned char> decode_data_url_base64(const char* input) {
    const char* base64_start = std::strstr(input, ";base64,");
    if (!base64_start)
        return {};

    base64_start += 8;

    size_t base64_len = strlen(base64_start);
    if (base64_len == 0)
        return {};

    return base64_decode(base64_start, base64_len);
}

cairo_surface_t* create_image_from_data(unsigned char* buf, size_t size) {
    cairo_surface_t* surface = nullptr;
    if (size > 8 && memcmp(buf, "\x89PNG\r\n\x1a\n", 8) == 0) {
        cairo_wrapper::BufferView view{buf, static_cast<unsigned int>(size), 0};
        surface = cairo_image_surface_create_from_png_stream(
            cairo_wrapper::read_from_view, &view);
    } else if (size > 3 && memcmp(buf, "\xFF\xD8\xFF", 3) == 0) {
        surface = cairo_wrapper::cairo_image_surface_create_from_jpeg_mem(buf, size);
    } else if (size > 12 && memcmp(buf, "RIFF", 4) == 0 &&
               memcmp(buf + 8, "WEBP", 4) == 0) {
        surface = cairo_wrapper::cairo_image_surface_create_from_webp_mem(buf, size);
    } else if (size > 3 && memcmp(buf, "GIF", 3) == 0) {
        surface = cairo_wrapper::cairo_image_surface_create_from_gif_mem(buf, size);
    } else if (size > 12 && memcmp(buf + 8, "avif", 4) == 0) {
        surface = cairo_wrapper::cairo_image_surface_create_from_avif_mem(buf, size);
    } else {
        return nullptr;
    }
    if (surface == nullptr) {
        return nullptr;
    }
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return nullptr;
    }
    return surface;
}

void htmlkit_container::load_image(const char* src, const char* baseurl,
                                   bool redraw_on_ready) {
    (void)redraw_on_ready;
    if (src == nullptr) {
        src = "";
    }
    if (baseurl == nullptr || !baseurl[0]) {
        baseurl = m_base_url.c_str();
    }
    if (strncmp(src, "data:", 5) == 0 && m_info.native_data_scheme) {
        auto decoded = decode_data_url_base64(src);
        if (!decoded.empty()) {
            if (cairo_surface_t* surface =
                    create_image_from_data(decoded.data(), decoded.size())) {
                m_img_surfaces.emplace(std::make_tuple(src, baseurl), surface);
                return;
            }
        }
    }
    std::string joined_url = call_urljoin(baseurl, src);
    auto image = m_resource_provider->fetch_image(joined_url);
    if (!image || image->empty()) {
        return;
    }
    if (cairo_surface_t* surface =
            create_image_from_data(image->data(), image->size())) {
        m_img_surfaces.emplace(std::make_tuple(src, baseurl), surface);
        if (joined_url != src) {
            m_img_surfaces.emplace(std::make_tuple(joined_url, baseurl),
                                   cairo_surface_reference(surface));
        }
    }
}

cairo_surface_t* htmlkit_container::get_image(const char* url, const char* baseurl) {
    if (url == nullptr) {
        url = "";
    }
    if (baseurl == nullptr || !baseurl[0]) {
        baseurl = m_base_url.c_str();
    }
    auto surface_it = m_img_surfaces.find(std::make_tuple(url, baseurl));
    if (surface_it != m_img_surfaces.end()) {
        return cairo_surface_reference(surface_it->second);
    }
    std::string joined_url = call_urljoin(baseurl, url);
    surface_it = m_img_surfaces.find(std::make_tuple(joined_url, baseurl));
    if (surface_it != m_img_surfaces.end()) {
        return cairo_surface_reference(surface_it->second);
    }
    return nullptr;
}

std::function<void()> htmlkit_container::import_css(
    const litehtml::string& url, const litehtml::string& origin_baseurl,
    const std::function<void(const litehtml::string& css_text,
                             const litehtml::string& new_baseurl)>& on_imported) {

    litehtml::string base_url = origin_baseurl.empty() ? m_base_url : origin_baseurl;

    auto empty = [=]() { on_imported("", base_url); };

    if (strncmp(url.c_str(), "data:", 5) == 0 && m_info.native_data_scheme) {
        auto decoded = decode_data_url_base64(url.c_str());
        if (decoded.empty()) {
            return empty;
        }
        litehtml::string css_text(reinterpret_cast<char*>(decoded.data()),
                                  decoded.size());
        return [=]() { on_imported(css_text, base_url); };
    }

    std::string joined_url = call_urljoin(base_url.c_str(), url.c_str());
    return [=]() {
        auto css_text = m_resource_provider->fetch_css(joined_url);
        if (!css_text) {
            on_imported("", joined_url);
        } else {
            on_imported(*css_text, joined_url);
        }
    };
}

std::string htmlkit_container::call_urljoin(const char* base, const char* url) {
    return m_resource_provider->join_url(base ? base : "", url ? url : "");
}

#pragma endregion
