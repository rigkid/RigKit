#include "XmlUtil.h"

#include <cctype>

namespace rigkit {
namespace ufo {

std::string xmlEscape(std::string_view s) {
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
			out += "&apos;";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}

std::string xmlUnescape(std::string_view s) {
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '&') {
			if (s.substr(i, 5) == "&amp;") {
				out += '&';
				i += 4;
			} else if (s.substr(i, 4) == "&lt;") {
				out += '<';
				i += 3;
			} else if (s.substr(i, 4) == "&gt;") {
				out += '>';
				i += 3;
			} else if (s.substr(i, 6) == "&quot;") {
				out += '"';
				i += 5;
			} else if (s.substr(i, 6) == "&apos;") {
				out += '\'';
				i += 5;
			} else {
				out += s[i];
			}
		} else {
			out += s[i];
		}
	}
	return out;
}

std::string xmlAttr(std::string_view raw, const char* key) {
	const std::string needle = std::string(key) + "=\"";
	const auto pos = raw.find(needle);
	if (pos == std::string_view::npos) {
		return {};
	}
	const size_t start = pos + needle.size();
	const auto end = raw.find('"', start);
	if (end == std::string_view::npos) {
		return {};
	}
	return xmlUnescape(raw.substr(start, end - start));
}

bool skipWs(std::string_view t, size_t& i) {
	while (i < t.size() && std::isspace(static_cast<unsigned char>(t[i]))) {
		++i;
	}
	return i < t.size();
}

bool consume(std::string_view t, size_t& i, std::string_view lit) {
	if (t.substr(i, lit.size()) != lit) {
		return false;
	}
	i += lit.size();
	return true;
}

std::string innerText(std::string_view t, size_t& i, std::string_view closeTag) {
	const auto end = t.find(closeTag, i);
	if (end == std::string_view::npos) {
		return {};
	}
	auto body = t.substr(i, end - i);
	i = end + closeTag.size();
	return xmlUnescape(body);
}

} // namespace ufo
} // namespace rigkit
