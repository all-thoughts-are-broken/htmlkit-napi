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

#ifndef HTMLKIT_CONTAINER_H
#define HTMLKIT_CONTAINER_H

#include <cairo.h>
#include <litehtml.h>
#include <map>
#include <set>
#include <utility>

#include "cairo_wrapper.h"
#include "container_info.h"
#include "resource_provider.h"

class htmlkit_container : public litehtml::document_container {
    cairo_wrapper::clip_box::vector m_clips;
    cairo_surface_t* m_temp_surface;
    cairo_t* m_temp_cr;
    std::set<std::string> m_all_fonts;
    std::string m_base_url;

    container_info m_info;

    std::map<std::tuple<std::string, std::string>, cairo_surface_t*> m_img_surfaces;
    ResourceProvider* m_resource_provider;

  public:
    htmlkit_container(const std::string& base_url, const container_info& info,
                      ResourceProvider* resource_provider);
    ~htmlkit_container() override;

    litehtml::uint_ptr create_font(const litehtml::font_description& descr,
                                   const litehtml::document* doc,
                                   litehtml::font_metrics* fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position& pos) override;
    litehtml::pixel_t pt_to_px(float pt) const override;
    litehtml::pixel_t get_default_font_size() const override;
    const char* get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc,
                          const litehtml::list_marker& marker) override;
    void load_image(const char* src, const char* baseurl,
                    bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl,
                        litehtml::size& sz) override;
    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
                    const std::string& url, const std::string& base_url) override;
    void draw_solid_fill(litehtml::uint_ptr hdc,
                         const litehtml::background_layer& layer,
                         const litehtml::web_color& color) override;
    void draw_linear_gradient(
        litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
        const litehtml::background_layer::linear_gradient& gradient) override;
    void draw_radial_gradient(
        litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
        const litehtml::background_layer::radial_gradient& gradient) override;
    void draw_conic_gradient(
        litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
        const litehtml::background_layer::conic_gradient& gradient) override;
    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
                      const litehtml::position& draw_pos, bool root) override;

    void set_caption(const char* caption) override;
    void set_base_url(const char* base_url) override;
    void link(const std::shared_ptr<litehtml::document>& doc,
              const litehtml::element::ptr& el) override;
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    bool on_element_click(const litehtml::element::ptr&) override;
    void on_mouse_event(const litehtml::element::ptr& el,
                        litehtml::mouse_event event) override;
    void set_cursor(const char* cursor) override;
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
    void set_clip(const litehtml::position& pos,
                  const litehtml::border_radiuses& bdr_radius) override;
    void del_clip() override;
    void get_viewport(litehtml::position& viewport) const override;
    void get_language(litehtml::string& language,
                      litehtml::string& culture) const override;
    void get_media_features(litehtml::media_features& media) const override;
    litehtml::element::ptr
    create_element(const char* tag_name, const litehtml::string_map& attributes,
                   const std::shared_ptr<litehtml::document>& doc) override;
    // litehtml::string resolve_color(const litehtml::string&) const override;
    // void split_text(const char* text, const std::function<void(const char*)>&
    // on_word, const std::function<void(const char*)>& on_space) override;

    std::function<void()>
    import_css(const litehtml::string& url, const litehtml::string& baseurl,
               const std::function<void(const litehtml::string& css_text,
                                        const litehtml::string& new_baseurl)>&
                   on_imported) override;
    cairo_surface_t* get_image(const char* url, const char* baseurl);

  protected:
    void draw_ellipse(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                      litehtml::pixel_t width, litehtml::pixel_t height,
                      const litehtml::web_color& color, litehtml::pixel_t line_width);
    void fill_ellipse(cairo_t* cr, litehtml::pixel_t x, litehtml::pixel_t y,
                      litehtml::pixel_t width, litehtml::pixel_t height,
                      const litehtml::web_color& color);
    void rounded_rectangle(cairo_t* cr, const litehtml::position& pos,
                           const litehtml::border_radiuses& radius);

    void clip_background_layer(cairo_t* cr, const litehtml::background_layer& layer);
    void apply_clip(cairo_t* cr);

    static void set_color(cairo_t* cr, const litehtml::web_color& color) {
        cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0,
                              color.blue / 255.0, color.alpha / 255.0);
    }

  private:
    static void add_path_arc(cairo_t* cr, double x, double y, double rx, double ry,
                             double a1, double a2, bool neg);
    static void draw_bmp(cairo_t* cr, cairo_surface_t* bmp, litehtml::pixel_t x,
                         litehtml::pixel_t y, int cx, int cy);
    static cairo_surface_t* scale_surface(cairo_surface_t* surface, int width,
                                          int height);
    std::string call_urljoin(const char* base, const char* url);
};

#endif // HTMLKIT_CONTAINER_H
