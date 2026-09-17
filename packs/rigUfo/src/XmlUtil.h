#pragma once

#include <string>
#include <string_view>

namespace rigkit {
namespace ufo {

std::string xmlEscape(std::string_view s);
std::string xmlAttr(std::string_view raw, const char* key);
bool skipWs(std::string_view t, size_t& i);
bool consume(std::string_view t, size_t& i, std::string_view lit);
std::string innerText(std::string_view t, size_t& i, std::string_view closeTag);

} // namespace ufo
} // namespace rigkit
