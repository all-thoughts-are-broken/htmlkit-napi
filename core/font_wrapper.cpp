#include "font_wrapper.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fontconfig/fontconfig.h>
#include <fstream>
#include <pango/pangocairo.h>
#include <shared_mutex>
#include <string>
#include <vector>

static PangoFontMap* global_fontmap = nullptr;
static std::shared_mutex global_fontmap_mutex;

namespace {
bool has_env_value(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
}

#ifdef _WIN32
std::string set_env_value(const char* name, const std::string& value) {
    if (_putenv_s(name, value.c_str()) != 0) {
        return {};
    }
    return value;
}

std::string join_path(const char* base, const char* suffix) {
    if (base == nullptr || base[0] == '\0') {
        return {};
    }

    std::string path(base);
    if (!path.empty() && path.back() != '\\' && path.back() != '/') {
        path += '\\';
    }
    path += suffix;
    return path;
}

std::string xml_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        switch (c) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        case '\'':
            escaped += "&apos;";
            break;
        default:
            escaped += c;
            break;
        }
    }
    return escaped;
}

std::string create_windows_font_config_xml(const std::string& cache_dir = {}) {
    std::vector<std::string> font_dirs;
    auto add_unique_dir = [&font_dirs](std::string dir) {
        if (!dir.empty() &&
            std::find(font_dirs.begin(), font_dirs.end(), dir) == font_dirs.end()) {
            font_dirs.push_back(std::move(dir));
        }
    };
    add_unique_dir(join_path(std::getenv("WINDIR"), "Fonts"));
    add_unique_dir(join_path(std::getenv("SystemRoot"), "Fonts"));
    add_unique_dir(
        join_path(std::getenv("LOCALAPPDATA"), "Microsoft\\Windows\\Fonts"));

    std::string xml =
        "<?xml version=\"1.0\"?>\n"
        "<fontconfig>\n";
    for (const auto& dir : font_dirs) {
        xml += "  <dir>";
        xml += xml_escape(dir);
        xml += "</dir>\n";
    }
    if (!cache_dir.empty()) {
        xml += "  <cachedir>";
        xml += xml_escape(cache_dir);
        xml += "</cachedir>\n";
    }
    xml += "</fontconfig>\n";
    return xml;
}

bool ensure_windows_fontconfig_file() {
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path config_dir = fs::temp_directory_path(ec) / "htmlkit-napi";
    if (ec) {
        return false;
    }

    fs::create_directories(config_dir, ec);
    if (ec) {
        return false;
    }

    fs::path config_file = config_dir / "fonts.conf";
    fs::path cache_dir = config_dir / "cache";
    fs::create_directories(cache_dir, ec);
    if (ec) {
        return false;
    }

    {
        std::ofstream output(config_file, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output << create_windows_font_config_xml(cache_dir.string());
    }

    return !set_env_value("FONTCONFIG_FILE", config_file.string()).empty();
}

FcConfig* create_windows_font_config() {
    FcConfig* cfg = FcConfigCreate();
    if (!cfg) {
        return nullptr;
    }

    const std::string xml = create_windows_font_config_xml();
    FcConfigParseAndLoadFromMemory(
        cfg, reinterpret_cast<const FcChar8*>(xml.c_str()), FcFalse);

    if (FcConfigBuildFonts(cfg) != FcTrue) {
        FcConfigDestroy(cfg);
        return nullptr;
    }

    return cfg;
}
#endif

FcConfig* load_font_config() {
#ifdef _WIN32
    if (!has_env_value("FONTCONFIG_FILE") && !has_env_value("FONTCONFIG_PATH")) {
        ensure_windows_fontconfig_file();
    }
#endif

    FcConfig* cfg = FcInitLoadConfigAndFonts();
    if (!cfg) {
#ifdef _WIN32
        cfg = create_windows_font_config();
#else
        cfg = FcConfigCreate();
#endif
    }
    return cfg;
}
} // namespace

int init_fontconfig() {
    FcConfig* cfg = load_font_config();
    if (!cfg) {
        return -1;
    }
    if (FcConfigSetCurrent(cfg) != FcTrue) {
        FcConfigDestroy(cfg);
        return -1;
    }
    global_fontmap_mutex.lock();
    if (global_fontmap != nullptr) {
        g_object_unref(global_fontmap);
    }
    global_fontmap = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    g_object_ref_sink(global_fontmap);
    global_fontmap_mutex.unlock();
    return 0;
}
