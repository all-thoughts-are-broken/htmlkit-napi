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

#include "cairo_wrapper.h"

#include <avif/avif.h>
#include <cmath>
#include <gif_lib.h>
#include <jpeglib.h>
#include <webp/decode.h>
#include <webp/demux.h>

using namespace cairo_wrapper;

#if defined(_WIN32) ||                                                           \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&              \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define HTMLKIT_LITTLE_ENDIAN 1
#else
#define HTMLKIT_LITTLE_ENDIAN 0
#endif

void border::cairo_add_path_arc(cairo_t* cr, double x, double y, double rx, double ry,
                                double a1, double a2, bool neg) {
    if (rx > 0 && ry > 0) {
        cairo_save(cr);

        cairo_translate(cr, x, y);
        cairo_scale(cr, 1.0, ry / rx);
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

void border::cairo_set_color(cairo_t* cr, const litehtml::web_color& color) {
    cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0,
                          color.blue / 255.0, color.alpha / 255.0);
}

void border::draw_border() {
    cairo_save(cr);

    if (radius_top_x && radius_top_y) {
        double start_angle = M_PI;
        double end_angle =
            start_angle + M_PI / 2.0 / (top_border_width / (double)border_width + 1);

        cairo_add_path_arc(
            cr, left + radius_top_x, top + radius_top_y, radius_top_x - border_width,
            radius_top_y - border_width + (border_width - top_border_width),
            start_angle, end_angle, false);

        cairo_add_path_arc(cr, left + radius_top_x, top + radius_top_y, radius_top_x,
                           radius_top_y, end_angle, start_angle, true);
    } else {
        cairo_move_to(cr, left + border_width, top + top_border_width);
        cairo_line_to(cr, left, top);
    }

    if (radius_bottom_x && radius_bottom_y) {
        cairo_line_to(cr, left, bottom - radius_bottom_y);

        double end_angle = M_PI;
        double start_angle =
            end_angle -
            M_PI / 2.0 / ((double)bottom_border_width / (double)border_width + 1);

        cairo_add_path_arc(cr, left + radius_bottom_x, bottom - radius_bottom_y,
                           radius_bottom_x, radius_bottom_y, end_angle, start_angle,
                           true);

        cairo_add_path_arc(cr, left + radius_bottom_x, bottom - radius_bottom_y,
                           radius_bottom_x - border_width,
                           radius_bottom_y - border_width +
                               (border_width - bottom_border_width),
                           start_angle, end_angle, false);
    } else {
        cairo_line_to(cr, left, bottom);
        cairo_line_to(cr, left + border_width, bottom - bottom_border_width);
    }
    cairo_clip(cr);

    switch (style) {
    case litehtml::border_style_dotted:
        draw_dotted();
        break;
    case litehtml::border_style_dashed:
        draw_dashed();
        break;
    case litehtml::border_style_double:
        draw_double();
        break;
    case litehtml::border_style_inset:
        draw_inset_outset(true);
        break;
    case litehtml::border_style_outset:
        draw_inset_outset(false);
        break;
    case litehtml::border_style_groove:
        draw_groove_ridge(true);
        break;
    case litehtml::border_style_ridge:
        draw_groove_ridge(false);
        break;
    default:
        draw_solid();
        break;
    }

    cairo_restore(cr);
}

void border::draw_line(double line_offset, double top_line_offset,
                       double bottom_line_offset) {
    if (radius_top_x && radius_top_y) {
        double end_angle = M_PI;
        double start_angle =
            end_angle +
            M_PI / 2.0 / ((double)top_border_width / (double)border_width + 1);

        cairo_add_path_arc(cr, left + radius_top_x, top + radius_top_y,
                           radius_top_x - line_offset,
                           radius_top_y - line_offset + (line_offset - top_line_offset),
                           start_angle, end_angle, true);
    } else {
        cairo_move_to(cr, left + line_offset, top);
    }

    if (radius_bottom_x && radius_bottom_y) {
        cairo_line_to(cr, left + line_offset, bottom - radius_bottom_y);

        double start_angle = M_PI;
        double end_angle =
            start_angle -
            M_PI / 2.0 / ((double)bottom_border_width / (double)border_width + 1);

        cairo_add_path_arc(cr, left + radius_bottom_x, bottom - radius_bottom_y,
                           radius_bottom_x - line_offset,
                           radius_bottom_y - line_offset +
                               (line_offset - bottom_line_offset),
                           start_angle, end_angle, true);
    } else {
        cairo_line_to(cr, left + line_offset, bottom);
    }
}

void border::draw_inset_outset(bool is_inset) {
    litehtml::web_color line_color;
    litehtml::web_color light_color = color;
    litehtml::web_color dark_color = color.darken(0.33);
    if (color.red == 0 && color.green == 0 && color.blue == 0) {
        dark_color.red = dark_color.green = dark_color.blue = 0x4C;
        light_color.red = light_color.green = light_color.blue = 0xB2;
    }

    if (side == left_side || side == top_side) {
        line_color = is_inset ? dark_color : light_color;
    } else {
        line_color = is_inset ? light_color : dark_color;
    }
    draw_line(border_width / 2.0, top_border_width / 2.0, bottom_border_width / 2.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_dash(cr, nullptr, 0, 0);
    cairo_set_color(cr, line_color);
    cairo_set_line_width(cr, border_width);
    cairo_stroke(cr);
}

void border::draw_double() {
    if (border_width < 3) {
        draw_solid();
    } else {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_set_dash(cr, nullptr, 0, 0);
        cairo_set_color(cr, color);

        double line_width = border_width / 3.0;
        cairo_set_line_width(cr, line_width);
        // draw external line
        draw_line(line_width / 2.0, top_border_width / 6.0, bottom_border_width / 6.0);
        cairo_stroke(cr);
        // draw internal line
        draw_line(border_width - line_width / 2.0,
                  top_border_width - top_border_width / 6.0,
                  bottom_border_width - bottom_border_width / 6.0);
        cairo_stroke(cr);
    }
}

void border::draw_dashed() {
    int line_length = std::abs(bottom - top);
    if (!line_length)
        return;

    draw_line(border_width / 2.0, top_border_width / 2.0, bottom_border_width / 2.0);

    int segment_length = border_width * 3;
    int seg_nums = line_length / segment_length;
    if (seg_nums < 2) {
        seg_nums = 2;
    }
    if (seg_nums % 2 != 0) {
        seg_nums = seg_nums + 1;
    }
    seg_nums++;
    double seg_len = (double)line_length / (double)seg_nums;

    double dashes[2];
    dashes[0] = seg_len;
    dashes[1] = seg_len;
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_set_color(cr, color);
    cairo_set_line_width(cr, border_width);
    cairo_stroke(cr);
}

void border::draw_solid() {
    draw_line(border_width / 2.0, top_border_width / 2.0, bottom_border_width / 2.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_dash(cr, nullptr, 0, 0);
    cairo_set_color(cr, color);
    cairo_set_line_width(cr, border_width);
    cairo_stroke(cr);
}

void border::draw_dotted() {
    // Zero length line
    if (bottom == top)
        return;

    draw_line(border_width / 2.0, top_border_width / 2.0, bottom_border_width / 2.0);

    double line_length = std::abs(bottom - top);

    double dot_size = border_width;
    int num_dots = (int)std::nearbyint(line_length / (dot_size * 2.0));
    if (num_dots < 2) {
        num_dots = 2;
    }
    if (num_dots % 2 != 0) {
        num_dots = num_dots + 1;
    }
    num_dots++;
    double space_len = ((double)line_length - (double)border_width) / (num_dots - 1.0);

    double dashes[2];
    dashes[0] = 0;
    dashes[1] = space_len;
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_dash(cr, dashes, 2, -dot_size / 2.0);

    cairo_set_color(cr, color);
    cairo_set_line_width(cr, border_width);
    cairo_stroke(cr);
}

void border::draw_groove_ridge(bool is_groove) {
    if (border_width == 1) {
        draw_solid();
    } else {
        litehtml::web_color inner_line_color;
        litehtml::web_color outer_line_color;
        litehtml::web_color light_color = color;
        litehtml::web_color dark_color = color.darken(0.33);
        if (color.red == 0 && color.green == 0 && color.blue == 0) {
            dark_color.red = dark_color.green = dark_color.blue = 0x4C;
            light_color.red = light_color.green = light_color.blue = 0xB2;
        }

        if (side == left_side || side == top_side) {
            outer_line_color = is_groove ? dark_color : light_color;
            inner_line_color = is_groove ? light_color : dark_color;
        } else {
            outer_line_color = is_groove ? light_color : dark_color;
            inner_line_color = is_groove ? dark_color : light_color;
        }

        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_set_dash(cr, nullptr, 0, 0);

        double line_width = border_width / 2.0;
        cairo_set_line_width(cr, line_width);
        // draw external line
        draw_line(line_width / 2.0, top_border_width / 4.0, bottom_border_width / 4.0);
        cairo_set_color(cr, outer_line_color);
        cairo_stroke(cr);
        // draw internal line
        cairo_set_color(cr, inner_line_color);
        draw_line(border_width - line_width / 2.0,
                  top_border_width - top_border_width / 4.0,
                  bottom_border_width - bottom_border_width / 4.0);
        cairo_stroke(cr);
    }
}

void conic_gradient::sector_patch(cairo_pattern_t* pat, const double radius,
                                  const double angle_A, const litehtml::web_color& A,
                                  const double angle_B, const litehtml::web_color& B) {
    const double A_r = A.red / 255.0, A_g = A.green / 255.0, A_b = A.blue / 255.0,
                 A_a = A.alpha / 255.0;
    const double B_r = B.red / 255.0, B_g = B.green / 255.0, B_b = B.blue / 255.0,
                 B_a = B.alpha / 255.0;

    const double r_sin_A = radius * sin(angle_A), r_cos_A = radius * cos(angle_A);
    const double r_sin_B = radius * sin(angle_B), r_cos_B = radius * cos(angle_B);

    const double h = 4.0 / 3.0 * tan((angle_B - angle_A) / 4.0);

    const double x0 = r_cos_A, y0 = r_sin_A;
    const double x1 = r_cos_A - h * r_sin_A, y1 = r_sin_A + h * r_cos_A;
    const double x2 = r_cos_B + h * r_sin_B, y2 = r_sin_B - h * r_cos_B;
    const double x3 = r_cos_B, y3 = r_sin_B;

    cairo_mesh_pattern_begin_patch(pat);

    cairo_mesh_pattern_move_to(pat, 0, 0);
    cairo_mesh_pattern_line_to(pat, x0, y0);

    cairo_mesh_pattern_curve_to(pat, x1, y1, x2, y2, x3, y3);
    cairo_mesh_pattern_line_to(pat, 0, 0);

    cairo_mesh_pattern_set_corner_color_rgba(pat, 0, A_r, A_g, A_b, A_a);
    cairo_mesh_pattern_set_corner_color_rgba(pat, 1, A_r, A_g, A_b, A_a);
    cairo_mesh_pattern_set_corner_color_rgba(pat, 2, B_r, B_g, B_b, B_a);
    cairo_mesh_pattern_set_corner_color_rgba(pat, 3, B_r, B_g, B_b, B_a);

    cairo_mesh_pattern_end_patch(pat);
}

cairo_pattern_t*
conic_gradient::create_pattern(double angle, double radius,
                               const std::vector<bg_color_point>& color_points) {
    if (color_points.empty()) {
        return nullptr;
    }

    if (color_points.size() == 1) {
        return create_pattern(angle, radius,
                              {color_points[0], color_points[0], color_points[0]});
    }

    if (color_points.size() == 2) {
        bg_color_point mid;
        const litehtml::web_color &a = color_points[0].color, b = color_points[1].color;
        mid.offset = 0.5;
        mid.color.red = interpolate(a.red, b.red);
        mid.color.green = interpolate(a.green, b.green);
        mid.color.blue = interpolate(a.blue, b.blue);
        mid.color.alpha = interpolate(a.alpha, b.alpha);
        return create_pattern(angle, radius, {color_points[0], mid, color_points[1]});
    }

    const double two_pi = 2.0 * M_PI;

    cairo_pattern_t* pat = cairo_pattern_create_mesh();

    for (size_t i = 0; i < color_points.size() - 1; i++) {
        const bg_color_point &cp_A = color_points[i], cp_B = color_points[i + 1];
        double angle_A = cp_A.offset * two_pi + angle;
        double angle_B = cp_B.offset * two_pi + angle;
        sector_patch(pat, radius, angle_A, cp_A.color, angle_B, cp_B.color);
    }

    return pat;
}

cairo_status_t cairo_wrapper::write_to_vector(void* closure, const unsigned char* data,
                                              unsigned int length) {
    auto* vec = static_cast<std::vector<unsigned char>*>(closure);
    if (!vec) {
        return CAIRO_STATUS_DEVICE_ERROR;
    }
    try {
        vec->insert(vec->end(), data, data + length);
    } catch (const std::bad_alloc&) {
        return CAIRO_STATUS_NO_MEMORY;
    } catch (...) {
        return CAIRO_STATUS_DEVICE_ERROR;
    }
    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t cairo_wrapper::read_from_view(void* closure, unsigned char* buffer,
                                             unsigned int length) {
    BufferView* view = static_cast<BufferView*>(closure);
    if (!view || view->size <= 0 || !view->data || view->offset < 0 ||
        view->offset > view->size) {
        return CAIRO_STATUS_READ_ERROR;
    }
    if (view->offset + length > view->size) {
        return CAIRO_STATUS_READ_ERROR;
    }
    memcpy((void*)buffer, view->data + view->offset, length);
    view->offset += length;
    return CAIRO_STATUS_SUCCESS;
}

/* Copyright 2018-2025 Bernhard R. Fischer, 4096R/8E24F29D <bf@abenteuerland.at>
 *
 * This file is part of Cairo_JPG.
 *
 * Cairo_JPG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cairo_JPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cairo_JPG.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! This function creates a JPEG file in memory from a Cairo image surface.
 * @param sfc Pointer to a Cairo surface. It should be an image surface of
 * either CAIRO_FORMAT_ARGB32 or CAIRO_FORMAT_RGB24. Other formats are
 * converted to CAIRO_FORMAT_RGB24 before compression.
 * Please note that this may give unexpected results because JPEG does not
 * support transparency. Thus, default background color is used to replace
 * transparent regions. The default background color is black if not specified
 * explicitly. Thus converting e.g. PDF surfaces without having any specific
 * background color set will apear with black background and not white as you
 * might expect. In such cases it is suggested to manually convert the surface
 * to RGB24 before calling this function.
 * @param data Pointer to a memory pointer. This parameter receives a pointer
 * to the memory area where the final JPEG data is found in memory. This
 * function reserves the memory properly and it has to be freed by the caller
 * with free(3).
 * @param len Pointer to a variable of type size_t which will receive the final
 * lenght of the memory buffer.
 * @param quality Compression quality, 0-100.
 * @return On success the function returns CAIRO_STATUS_SUCCESS. In case of
 * error CAIRO_STATUS_INVALID_FORMAT is returned.
 */
cairo_status_t cairo_wrapper::cairo_surface_write_to_jpeg_mem(cairo_surface_t* sfc,
                                                              unsigned char** data,
                                                              size_t* len,
                                                              int quality) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    cairo_surface_t* other = NULL;

    // check valid input format (must be IMAGE_SURFACE && (ARGB32 || RGB24))
    if (cairo_surface_get_type(sfc) != CAIRO_SURFACE_TYPE_IMAGE ||
        (cairo_image_surface_get_format(sfc) != CAIRO_FORMAT_ARGB32 &&
         cairo_image_surface_get_format(sfc) != CAIRO_FORMAT_RGB24)) {
        // create a similar surface with a proper format if supplied input format
        // does not fulfill the requirements
        double x1, y1, x2, y2;
        other = sfc;
        cairo_t* ctx = cairo_create(other);
        // get extents of original surface
        cairo_clip_extents(ctx, &x1, &y1, &x2, &y2);
        cairo_destroy(ctx);

        // create new image surface
        sfc = cairo_surface_create_similar_image(other, CAIRO_FORMAT_RGB24, x2 - x1,
                                                 y2 - y1);
        if (cairo_surface_status(sfc) != CAIRO_STATUS_SUCCESS)
            return CAIRO_STATUS_INVALID_FORMAT;

        // paint original surface to new surface
        ctx = cairo_create(sfc);
        cairo_set_source_surface(ctx, other, 0, 0);
        cairo_paint(ctx);
        cairo_destroy(ctx);
    }

    // finish queued drawing operations
    cairo_surface_flush(sfc);

    // init jpeg compression structures
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // set compression parameters
    unsigned long jpeg_len = *len;
    jpeg_mem_dest(&cinfo, data, &jpeg_len);
    cinfo.image_width = cairo_image_surface_get_width(sfc);
    cinfo.image_height = cairo_image_surface_get_height(sfc);
#if HTMLKIT_LITTLE_ENDIAN
    // cinfo.in_color_space = JCS_EXT_BGRX;
    cinfo.in_color_space = cairo_image_surface_get_format(sfc) == CAIRO_FORMAT_ARGB32
                               ? JCS_EXT_BGRA
                               : JCS_EXT_BGRX;
#else
    // cinfo.in_color_space = JCS_EXT_XRGB;
    cinfo.in_color_space = cairo_image_surface_get_format(sfc) == CAIRO_FORMAT_ARGB32
                               ? JCS_EXT_ARGB
                               : JCS_EXT_XRGB;
#endif
    cinfo.input_components = 4;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    // start compressor
    jpeg_start_compress(&cinfo, TRUE);

    unsigned char* pixels = cairo_image_surface_get_data(sfc);
    int stride = cairo_image_surface_get_stride(sfc);

    // loop over all lines and compress
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = pixels + (cinfo.next_scanline * stride);
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // finalize and close everything
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    *len = jpeg_len;

    // destroy temporary image surface (if available)
    if (other != NULL)
        cairo_surface_destroy(sfc);

    return CAIRO_STATUS_SUCCESS;
}

/*! This function decompresses a JPEG image from a memory buffer and creates a
 * Cairo image surface.
 * @param data Pointer to JPEG data (i.e. the full contents of a JPEG file read
 * into this buffer).
 * @param len Length of buffer in bytes.
 * @return Returns a pointer to a cairo_surface_t structure. It should be
 * checked with cairo_surface_status() for errors.
 */
cairo_surface_t* cairo_wrapper::cairo_image_surface_create_from_jpeg_mem(void* data,
                                                                         size_t len) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    cairo_surface_t* sfc;

    // initialize jpeg decompression structures
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (const unsigned char*)data, len);
    (void)jpeg_read_header(&cinfo, TRUE);

#if HTMLKIT_LITTLE_ENDIAN
    cinfo.out_color_space = JCS_EXT_BGRA;
#else
    cinfo.out_color_space = JCS_EXT_ARGB;
#endif

    // start decompressor
    (void)jpeg_start_decompress(&cinfo);

    // create Cairo image surface
    sfc = cairo_image_surface_create(CAIRO_FORMAT_RGB24, cinfo.output_width,
                                     cinfo.output_height);
    if (cairo_surface_status(sfc) != CAIRO_STATUS_SUCCESS) {
        jpeg_destroy_decompress(&cinfo);
        return sfc;
    }

    // loop over all scanlines and fill Cairo image surface
    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char* row_address =
            cairo_image_surface_get_data(sfc) +
            (cinfo.output_scanline * cairo_image_surface_get_stride(sfc));
        row_pointer[0] = row_address;
        (void)jpeg_read_scanlines(&cinfo, row_pointer, 1);
    }

    // finish and close everything
    cairo_surface_mark_dirty(sfc);
    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return sfc;
}

cairo_surface_t*
cairo_wrapper::cairo_image_surface_create_from_webp_mem(const uint8_t* data,
                                                        size_t len) {
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(data, len, &features) != VP8_STATUS_OK)
        return nullptr;

    // --- Animated WebP path ---
    if (features.has_animation) {
        WebPData webp_data = {data, len};
        WebPAnimDecoderOptions opts;
        WebPAnimDecoderOptionsInit(&opts);
#if HTMLKIT_LITTLE_ENDIAN
        opts.color_mode = MODE_BGRA;
#else
        opts.color_mode = MODE_ARGB;
#endif
        WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &opts);
        if (!dec)
            return nullptr;

        WebPAnimInfo anim_info;
        if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
            WebPAnimDecoderDelete(dec);
            return nullptr;
        }

        int width = anim_info.canvas_width;
        int height = anim_info.canvas_height;

        cairo_surface_t* surface =
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            WebPAnimDecoderDelete(dec);
            return nullptr;
        }

        uint8_t* pixels = cairo_image_surface_get_data(surface);
        int stride = cairo_image_surface_get_stride(surface);

        // Get the *first frame*
        uint8_t* frame_rgba;
        int timestamp;
        if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &timestamp)) {
            cairo_surface_destroy(surface);
            WebPAnimDecoderDelete(dec);
            return nullptr;
        }

        for (int y = 0; y < height; y++) {
            memcpy(pixels + y * stride, frame_rgba + y * width * 4, width * 4);
        }

        cairo_surface_mark_dirty(surface);
        WebPAnimDecoderDelete(dec);
        return surface;
    }

    int width = features.width;
    int height = features.height;

    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        return nullptr;

    uint8_t* pixels = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

#if HTMLKIT_LITTLE_ENDIAN
    if (!WebPDecodeBGRAInto(data, len, pixels, stride * height, stride))
#else
    if (!WebPDecodeARGBInto(data, len, pixels, stride * height, stride))
#endif
    {
        cairo_surface_destroy(surface);
        return nullptr;
    }

    cairo_surface_mark_dirty(surface);
    return surface;
}

cairo_surface_t*
cairo_wrapper::cairo_image_surface_create_from_avif_mem(const uint8_t* data,
                                                        size_t len) {
    avifDecoder* decoder = avifDecoderCreate();
    if (avifDecoderSetIOMemory(decoder, data, len) != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return nullptr;
    }
    if (avifDecoderParse(decoder) != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return nullptr;
    }
    if (avifDecoderNextImage(decoder) != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    int width = decoder->image->width;
    int height = decoder->image->height;

    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    uint8_t* pixels = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.depth = 8;
#if HTMLKIT_LITTLE_ENDIAN
    rgb.format = AVIF_RGB_FORMAT_BGRA;
#else
    rgb.format = AVIF_RGB_FORMAT_ARGB;
#endif
    rgb.rowBytes = stride;
    rgb.pixels = pixels;

    if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK) {
        cairo_surface_destroy(surface);
        avifDecoderDestroy(decoder);
        return nullptr;
    }
    cairo_surface_mark_dirty(surface);

    avifDecoderDestroy(decoder);
    return surface;
}

cairo_surface_t*
cairo_wrapper::cairo_image_surface_create_from_gif_mem(const uint8_t* data,
                                                       size_t len) {
    int error = 0;
    size_t offset = 0;
    typedef struct {
        const uint8_t* data;
        size_t size;
        size_t* offset;
    } Context;
    Context ctx = {data, len, &offset};

    auto readFunc = [](GifFileType* gif, GifByteType* buf, int len) -> int {
        auto* ctx = static_cast<Context*>(gif->UserData);
        size_t remain = ctx->size - *ctx->offset;
        if (len > remain)
            len = remain;
        memcpy(buf, ctx->data + *ctx->offset, len);
        *ctx->offset += len;
        return len;
    };

    GifFileType* gif = DGifOpen(&ctx, readFunc, &error);
    if (!gif)
        return nullptr;

    DGifSlurp(gif);
    if (gif->ImageCount < 1) {
        DGifCloseFile(gif, &error);
        return nullptr;
    }

    SavedImage* frame = &gif->SavedImages[0];
    ColorMapObject* map =
        frame->ImageDesc.ColorMap ? frame->ImageDesc.ColorMap : gif->SColorMap;
    if (!map) {
        DGifCloseFile(gif, &error);
        return nullptr;
    }

    int width = gif->SWidth, height = gif->SHeight;
    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        DGifCloseFile(gif, &error);
        return nullptr;
    }

    uint8_t* pixels = cairo_image_surface_get_data(surface);
    for (int y = 0; y < height; y++) {
        uint32_t* row =
            (uint32_t*)(pixels + y * cairo_image_surface_get_stride(surface));
        for (int x = 0; x < width; x++) {
            int idx = frame->RasterBits[y * width + x];
            GifColorType c = map->Colors[idx];
            row[x] = (0xFF << 24) | (c.Red << 16) | (c.Green << 8) | c.Blue;
        }
    }

    cairo_surface_mark_dirty(surface);
    DGifCloseFile(gif, &error);
    return surface;
}
