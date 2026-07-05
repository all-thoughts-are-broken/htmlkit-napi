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

#ifndef CAIRO_WRAPPER_H
#define CAIRO_WRAPPER_H

#include <cairo.h>
#include <litehtml.h>
#include <pango/pango-font.h>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cairo_wrapper {
class border {
  public:
    enum side_t {
        left_side,
        top_side,
        right_side,
        bottom_side,
    };

    side_t side;
    litehtml::web_color color;
    litehtml::border_style style;

    int border_width;
    int top_border_width;
    int bottom_border_width;

    int radius_top_x;
    int radius_top_y;
    int radius_bottom_x;
    int radius_bottom_y;

    border(cairo_t* _cr, const int _left, const int _top, const int _bottom)
        : side(left_side), color(), style(litehtml::border_style_none), border_width(0),
          top_border_width(0), bottom_border_width(0), radius_top_x(0), radius_top_y(0),
          radius_bottom_x(0), radius_bottom_y(0), cr(_cr), left(_left), top(_top),
          bottom(_bottom) {}

    void draw_border();

  private:
    cairo_t* cr;
    int left;
    int top;
    int bottom;
    void draw_line(double line_offset, double top_line_offset,
                   double bottom_line_offset);
    void draw_solid();
    void draw_dotted();
    void draw_dashed();
    void draw_double();
    void draw_inset_outset(bool is_inset);
    void draw_groove_ridge(bool is_groove);

    static void cairo_add_path_arc(cairo_t* cr, double x, double y, double rx,
                                   double ry, double a1, double a2, bool neg);
    static void cairo_set_color(cairo_t* cr, const litehtml::web_color& color);
};

class surface_t {
    cairo_surface_t* surface;

  public:
    explicit surface_t(cairo_surface_t* v) : surface(v) {}

    surface_t() : surface(nullptr) {}

    surface_t(const surface_t& v) : surface(v.surface) {
        if (v.surface != nullptr) {
            surface = cairo_surface_reference(v.surface);
        }
    }

    surface_t(surface_t&& v) noexcept {
        surface = v.surface;
        v.surface = nullptr;
    }

    surface_t& operator=(const surface_t& v) noexcept {
        if (surface != v.surface) {
            if (surface != nullptr) {
                cairo_surface_destroy(surface);
            }
            surface = cairo_surface_reference(v.surface);
        }
        return *this;
    }

    ~surface_t() {
        if (surface != nullptr) {
            cairo_surface_destroy(surface);
        }
    }

    cairo_surface_t* get() const { return cairo_surface_reference(surface); }
};

struct clip_box {
    typedef std::vector<clip_box> vector;
    litehtml::position box;
    litehtml::border_radiuses radius;

    clip_box(const litehtml::position& vBox, const litehtml::border_radiuses& vRad) {
        box = vBox;
        radius = vRad;
    }

    clip_box(const clip_box& val) {
        box = val.box;
        radius = val.radius;
    }
    clip_box& operator=(const clip_box& val) {
        box = val.box;
        radius = val.radius;
        return *this;
    }
};

struct font_t {
    PangoFontDescription* font;
    int size;
    bool underline;
    bool strikeout;
    bool overline;
    int ascent;
    int descent;
    int underline_thickness;
    int underline_position;
    int strikethrough_thickness;
    int strikethrough_position;
    int overline_thickness;
    int overline_position;
    int decoration_style;
    litehtml::web_color decoration_color;
};

namespace conic_gradient {
using bg_color_point = litehtml::background_layer::color_point;

static double interpolate(double a, double b) { return a + (b - a) * 0.5; }

static void sector_patch(cairo_pattern_t* pat, double radius, double angle_A,
                         const litehtml::web_color& A, double angle_B,
                         const litehtml::web_color& B);
cairo_pattern_t* create_pattern(double angle, double radius,
                                const std::vector<bg_color_point>& color_points);
} // namespace conic_gradient

struct BufferView {
    const unsigned char* data;
    unsigned int size;
    unsigned int offset;
};

cairo_status_t write_to_vector(void* closure, const unsigned char* data,
                               unsigned int length);
cairo_status_t read_from_view(void* closure, unsigned char* buffer,
                              unsigned int length);
cairo_status_t cairo_surface_write_to_jpeg_mem(cairo_surface_t* sfc,
                                               unsigned char** data, size_t* len,
                                               int quality);
cairo_surface_t* cairo_image_surface_create_from_jpeg_mem(void* data, size_t len);

cairo_surface_t* cairo_image_surface_create_from_avif_mem(const uint8_t* data,
                                                          size_t len);

cairo_surface_t* cairo_image_surface_create_from_webp_mem(const uint8_t* data,
                                                          size_t len);

cairo_surface_t* cairo_image_surface_create_from_gif_mem(const uint8_t* data,
                                                         size_t len);
} // namespace cairo_wrapper

#endif // CAIRO_WRAPPER_H
