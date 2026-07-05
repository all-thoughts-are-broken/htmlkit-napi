#ifndef DEBUG_CONTAINER_H
#define DEBUG_CONTAINER_H

#include <cairo/cairo.h>
#include <string>
#include <tuple>
#include <vector>

#include "htmlkit_container.h"

using font_info =
    std::tuple<std::string, litehtml::web_color, int, bool, bool, bool, int>;

class debug_container : public htmlkit_container {
    cairo_surface_t* m_dbg_surface = nullptr;
    std::string m_dbg_text_buffer;
    font_info m_dbg_text_info;
    std::vector<std::tuple<std::vector<unsigned char>, std::string>> m_debug_layers;

    class managed_cairo_t {
      public:
        cairo_t* ptr;
        cairo_surface_t* surface;
        explicit managed_cairo_t(cairo_surface_t* surface) : surface(surface) {
            ptr = cairo_create(surface);
            cairo_save(ptr);
        }
        ~managed_cairo_t() {
            cairo_restore(ptr);
            cairo_surface_flush(surface);
            cairo_destroy(ptr);
        }
        litehtml::uint_ptr u_ptr() const {
            return reinterpret_cast<litehtml::uint_ptr>(ptr);
        }
    };

  public:
    debug_container(const std::string& base_url, const container_info& info,
                    ResourceProvider* resource_provider);
    std::string export_debug_layers();
    void set_debug_surface(cairo_surface_t* surface) { m_dbg_surface = surface; }
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                   litehtml::web_color color, const litehtml::position& pos) override;
    void draw_list_marker(litehtml::uint_ptr hdc,
                          const litehtml::list_marker& marker) override;
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

  protected:
    void clear_dbg_surface() const;
    void check_pending_text_draws();
    void save_dbg_surface(std::string info);
};

#endif // PLUGIN_HTMLKIT_DEBUG_CONTAINER_H
