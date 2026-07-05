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

#include "resource_provider.h"

#include <cctype>

static bool has_url_scheme(const std::string& url) {
    const auto pos = url.find(':');
    if (pos == std::string::npos || pos == 0) {
        return false;
    }
    for (size_t i = 0; i < pos; ++i) {
        const unsigned char c = static_cast<unsigned char>(url[i]);
        if (!std::isalnum(c) && url[i] != '+' && url[i] != '-' && url[i] != '.') {
            return false;
        }
    }
    return true;
}

std::string NullResourceProvider::join_url(const std::string& base,
                                           const std::string& url) {
    if (url.empty() || has_url_scheme(url) || url.rfind("//", 0) == 0) {
        return url;
    }
    if (base.empty()) {
        return url;
    }
    if (url.front() == '/') {
        const auto scheme = base.find("://");
        if (scheme == std::string::npos) {
            return url;
        }
        const auto origin_end = base.find('/', scheme + 3);
        return origin_end == std::string::npos ? base + url
                                               : base.substr(0, origin_end) + url;
    }

    const auto query_pos = base.find_first_of("?#");
    std::string clean_base =
        query_pos == std::string::npos ? base : base.substr(0, query_pos);
    const auto slash = clean_base.find_last_of('/');
    if (slash == std::string::npos || slash + 1 == clean_base.size()) {
        return clean_base + url;
    }
    return clean_base.substr(0, slash + 1) + url;
}

std::optional<std::vector<unsigned char>>
NullResourceProvider::fetch_image(const std::string&) {
    return std::nullopt;
}

std::optional<std::string> NullResourceProvider::fetch_css(const std::string&) {
    return std::nullopt;
}
