#include <fmt/core.h>
#include <vector>

#include "base64.h"
#include "cairo_wrapper.h"
#include "debug_container.h"

#include <pango/pango-font.h>

debug_container::debug_container(const std::string& base_url,
                                 const container_info& info,
                                 ResourceProvider* resource_provider)
    : htmlkit_container(base_url, info, resource_provider) {}

void debug_container::clear_dbg_surface() const {
    const managed_cairo_t cr(m_dbg_surface);
    cairo_set_operator(cr.ptr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr.ptr);
}

void debug_container::save_dbg_surface(std::string info) {
    std::vector<unsigned char> bytes;
    cairo_surface_write_to_png_stream(m_dbg_surface, cairo_wrapper::write_to_vector,
                                      &bytes);
    clear_dbg_surface();
    m_debug_layers.emplace_back(std::move(bytes), std::move(info));
}

static std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

std::string make_html(
    const std::vector<std::tuple<std::vector<unsigned char>, std::string>>& images) {
    std::string html;

    html += R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Stacked Images</title>
<style>
  body { font-family: sans-serif; margin: 0; }
  .container { position: relative; display: inline-block; margin-right: 260px; }
  .layer { position: absolute; top: 0; left: 0; }
  .sidebar {
    position: fixed;
    top: 0; right: 0;
    width: 250px;
    height: 100%;
    overflow-y: auto;
    background: #f9f9f9;
    border-left: 1px solid #ccc;
    padding: 10px;
  }
  .sidebar h2 { margin-top: 0; font-size: 16px; }
  .control-item { margin-bottom: 10px; }
</style>
</head>
<body>
<div class="container">
)";

    // embed each PNG
    for (size_t i = 0; i < images.size(); ++i) {
        const auto& data = std::get<0>(images[i]);

        std::string b64 = base64_encode(data.data(), data.size());

        html += fmt::format(
            R"(<img id="img{}" class="layer" src="data:image/png;base64,{}" style="opacity:1.0; display:block;">
)",
            i, b64);
    }

    html += R"(</div>
<div class="sidebar">
  <h2>Layers</h2>
)";

    // controls
    for (size_t i = 0; i < images.size(); ++i) {
        std::string label = html_escape(std::get<1>(images[i]));
        html += fmt::format(
            R"#(<div class="control-item"><b>{}</b><br>
<input type="checkbox" id="chk{}" checked onchange="toggleLayer({})"> show<br>
Opacity: <input type="range" id="rng{}" min="0" max="100" value="100" oninput="changeOpacity({}, this.value)">
</div>
)#",
            label, i, i, i, i);
    }

    html += R"(</div>
<script>
function toggleLayer(i) {
    var img = document.getElementById("img" + i);
    var chk = document.getElementById("chk" + i);
    img.style.display = chk.checked ? "block" : "none";
}
function changeOpacity(i, val) {
    var img = document.getElementById("img" + i);
    img.style.opacity = val / 100.0;
}
</script>
</body>
</html>
)";

    return html;
}

std::string debug_container::export_debug_layers() {
    check_pending_text_draws();
    return make_html(m_debug_layers);
}

using cairo_font = cairo_wrapper::font_t;

void debug_container::draw_text(litehtml::uint_ptr hdc, const char* text,
                                litehtml::uint_ptr hFont, litehtml::web_color color,
                                const litehtml::position& pos) {
    htmlkit_container::draw_text(hdc, text, hFont, color, pos);
    if (m_dbg_surface != nullptr) {
        auto* fnt = reinterpret_cast<cairo_font*>(hFont);
        const char* fnt_family = pango_font_description_get_family(fnt->font);
        if (!m_dbg_text_buffer.empty()) {
            m_dbg_text_buffer += " ";
        }
        font_info curr_info = {fnt_family,           color,          fnt->size,
                               fnt->strikeout,       fnt->underline, fnt->overline,
                               fnt->decoration_style};
        if (curr_info == m_dbg_text_info) {
            m_dbg_text_buffer += text;
        } else {
            check_pending_text_draws();
            m_dbg_text_info = curr_info;
            m_dbg_text_buffer = text;
        }
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_text(cr.u_ptr(), text, hFont, color, pos);
        }
    }
}

void debug_container::check_pending_text_draws() {
    if (!m_dbg_text_buffer.empty()) {
        auto t_info = m_dbg_text_info;
        std::string info = fmt::format("draw_text({}, {}, {}, {})", std::get<0>(t_info),
                                       std::get<1>(t_info).to_string(),
                                       std::get<2>(t_info), m_dbg_text_buffer);
        m_dbg_text_buffer.clear();
        save_dbg_surface(info);
    }
}

void debug_container::draw_list_marker(litehtml::uint_ptr hdc,
                                       const litehtml::list_marker& marker) {
    htmlkit_container::draw_list_marker(hdc, marker);
    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_list_marker(cr.u_ptr(), marker);
        }
        const char* arg = "";
        if (!marker.image.empty()) {
            arg = marker.image.c_str();
        } else {
            switch (marker.marker_type) {
            case litehtml::list_style_type_circle:
                arg = "circle";
                break;
            case litehtml::list_style_type_disc:
                arg = "disc";
                break;
            case litehtml::list_style_type_square:
                arg = "square";
                break;
            default:
                break;
            }
        }
        save_dbg_surface(fmt::format("draw_list_marker({})", arg));
    }
}

void debug_container::draw_image(litehtml::uint_ptr hdc,
                                 const litehtml::background_layer& layer,
                                 const std::string& url, const std::string& base_url) {
    htmlkit_container::draw_image(hdc, layer, url, base_url);
    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_image(cr.u_ptr(), layer, url, base_url);
        }
        save_dbg_surface(fmt::format("draw_image({})", url));
    }
}

void debug_container::draw_solid_fill(litehtml::uint_ptr hdc,
                                      const litehtml::background_layer& layer,
                                      const litehtml::web_color& color) {
    htmlkit_container::draw_solid_fill(hdc, layer, color);

    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_solid_fill(cr.u_ptr(), layer, color);
        }
        save_dbg_surface(fmt::format("draw_solid_fill({})", color.to_string()));
    }
}

void debug_container::draw_linear_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::linear_gradient& gradient) {
    htmlkit_container::draw_linear_gradient(hdc, layer, gradient);

    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_linear_gradient(cr.u_ptr(), layer, gradient);
        }
        save_dbg_surface("draw_linear_gradient()");
    }
}

void debug_container::draw_radial_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::radial_gradient& gradient) {
    htmlkit_container::draw_radial_gradient(hdc, layer, gradient);

    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_radial_gradient(cr.u_ptr(), layer, gradient);
        }
        save_dbg_surface("draw_radial_gradient()");
    }
}

void debug_container::draw_conic_gradient(
    litehtml::uint_ptr hdc, const litehtml::background_layer& layer,
    const litehtml::background_layer::conic_gradient& gradient) {
    htmlkit_container::draw_conic_gradient(hdc, layer, gradient);

    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_conic_gradient(cr.u_ptr(), layer, gradient);
        }
        save_dbg_surface("draw_conic_gradient()");
    }
}

void debug_container::draw_borders(litehtml::uint_ptr hdc,
                                   const litehtml::borders& borders,
                                   const litehtml::position& draw_pos, bool root) {
    htmlkit_container::draw_borders(hdc, borders, draw_pos, root);
    if (m_dbg_surface != nullptr) {
        check_pending_text_draws();
        {
            const managed_cairo_t cr(m_dbg_surface);
            htmlkit_container::draw_borders(cr.u_ptr(), borders, draw_pos, root);
        }
        save_dbg_surface(fmt::format("draw_borders(root={})", root));
    }
}
