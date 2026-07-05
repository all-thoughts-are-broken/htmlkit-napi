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

#include "render_engine.h"

#include <algorithm>
#include <cairo.h>
#include <cstdlib>
#include <litehtml.h>
#include <litehtml/render_item.h>
#include <pango/pangocairo.h>
#include <stdexcept>

#include "cairo_wrapper.h"
#include "container_info.h"
#include "debug_container.h"

namespace {
class FontOptionsGuard {
  public:
    explicit FontOptionsGuard(cairo_font_options_t* value) : value_(value) {}
    ~FontOptionsGuard() {
        if (value_ != nullptr) {
            cairo_font_options_destroy(value_);
        }
    }
    cairo_font_options_t* get() const { return value_; }

  private:
    cairo_font_options_t* value_;
};

class CairoSurfaceGuard {
  public:
    explicit CairoSurfaceGuard(cairo_surface_t* value) : value_(value) {}
    ~CairoSurfaceGuard() {
        if (value_ != nullptr) {
            cairo_surface_destroy(value_);
        }
    }
    cairo_surface_t* get() const { return value_; }
    cairo_surface_t* release() {
        auto* value = value_;
        value_ = nullptr;
        return value;
    }

  private:
    cairo_surface_t* value_;
};

class CairoGuard {
  public:
    explicit CairoGuard(cairo_surface_t* surface) : value_(cairo_create(surface)) {}
    ~CairoGuard() {
        if (value_ != nullptr) {
            cairo_destroy(value_);
        }
    }
    cairo_t* get() const { return value_; }

  private:
    cairo_t* value_;
};

class PangoFontMapGuard {
  public:
    PangoFontMapGuard()
        : value_(pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT)) {
        pango_cairo_font_map_set_default(PANGO_CAIRO_FONT_MAP(value_));
    }
    ~PangoFontMapGuard() {
        if (value_ != nullptr) {
            g_object_unref(value_);
        }
    }

  private:
    PangoFontMap* value_;
};

void throw_cairo_error(cairo_status_t status) {
    throw std::runtime_error(cairo_status_to_string(status));
}
} // namespace

RenderResult render_html(const RenderOptions& options,
                         ResourceProvider* resource_provider) {
    PangoFontMapGuard font_map;

    container_info info;
    info.dpi = options.dpi;
    info.width = options.width;
    info.height = options.height;
    info.default_font_size_pt = options.default_font_size;
    info.default_font_name = options.font_name;
    info.language = options.language;
    info.culture = options.culture;
    info.font_options = cairo_font_options_create();
    info.native_data_scheme = options.native_data_scheme;
    FontOptionsGuard font_options(info.font_options);

    cairo_font_options_set_antialias(info.font_options, CAIRO_ANTIALIAS_DEFAULT);
    cairo_font_options_set_hint_style(info.font_options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_subpixel_order(info.font_options,
                                          CAIRO_SUBPIXEL_ORDER_DEFAULT);

    debug_container container(options.base_url, info, resource_provider);

    auto doc = litehtml::document::createFromString(
        options.html, &container, litehtml::master_css,
        " html { background-color: #fff; }");

    int width = static_cast<int>(options.width);
    litehtml::pixel_t best_width = doc->render(options.width);
    if (options.allow_refit && best_width < options.width) {
        width = best_width;
        doc->render(width);
    }

    int content_height = doc->height();
    width = std::max(1, width);
    content_height = std::max(1, content_height);

    CairoSurfaceGuard surface(
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, content_height));
    CairoSurfaceGuard debug_surface(
        options.debug
            ? cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, content_height)
            : nullptr);
    container.set_debug_surface(debug_surface.get());

    if (surface.get() == nullptr) {
        throw std::runtime_error("Could not create surface");
    }
    if (cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS) {
        throw_cairo_error(cairo_surface_status(surface.get()));
    }
    if (debug_surface.get() != nullptr &&
        cairo_surface_status(debug_surface.get()) != CAIRO_STATUS_SUCCESS) {
        throw_cairo_error(cairo_surface_status(debug_surface.get()));
    }

    CairoGuard cr(surface.get());

    cairo_save(cr.get());
    cairo_rectangle(cr.get(), 0, 0, width, content_height);
    cairo_set_source_rgba(cr.get(), 1.0, 1.0, 1.0, 1.0);
    cairo_fill(cr.get());
    cairo_restore(cr.get());

    litehtml::position clip(0, 0, width, content_height);
    doc->draw(reinterpret_cast<litehtml::uint_ptr>(cr.get()), 0, 0, &clip);

    cairo_surface_flush(surface.get());

    RenderResult result;
    if (options.image_flag >= 0 && options.image_flag <= 100) {
        unsigned char* jpeg_data = nullptr;
        size_t jpeg_size = 0;
        cairo_status_t status = cairo_wrapper::cairo_surface_write_to_jpeg_mem(
            surface.get(), &jpeg_data, &jpeg_size, options.image_flag);
        if (status != CAIRO_STATUS_SUCCESS) {
            free(jpeg_data);
            throw_cairo_error(status);
        }
        result.image.assign(jpeg_data, jpeg_data + jpeg_size);
        free(jpeg_data);
    } else {
        cairo_status_t status = cairo_surface_write_to_png_stream(
            surface.get(), cairo_wrapper::write_to_vector, &result.image);
        if (status != CAIRO_STATUS_SUCCESS) {
            throw_cairo_error(status);
        }
    }

    if (options.debug) {
        result.debug_html = container.export_debug_layers();
    }

    return result;
}
