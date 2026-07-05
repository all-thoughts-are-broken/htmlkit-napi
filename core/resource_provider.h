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

#ifndef RESOURCE_PROVIDER_H
#define RESOURCE_PROVIDER_H

#include <optional>
#include <string>
#include <vector>

class ResourceProvider {
  public:
    virtual ~ResourceProvider() = default;

    virtual std::string join_url(const std::string& base,
                                 const std::string& url) = 0;
    virtual std::optional<std::vector<unsigned char>>
    fetch_image(const std::string& url) = 0;
    virtual std::optional<std::string> fetch_css(const std::string& url) = 0;
};

class NullResourceProvider : public ResourceProvider {
  public:
    std::string join_url(const std::string& base,
                         const std::string& url) override;
    std::optional<std::vector<unsigned char>>
    fetch_image(const std::string& url) override;
    std::optional<std::string> fetch_css(const std::string& url) override;
};

#endif // RESOURCE_PROVIDER_H
