#include "Plist.h"

#include "XmlUtil.h"

#include <sstream>

namespace rigkit {
namespace ufo {
namespace {

bool parseValue(std::string_view t, size_t& i, Plist& out);

bool parseDict(std::string_view t, size_t& i, Plist& out) {
	out.kind = Plist::Dict;
	out.dict.clear();
	while (skipWs(t, i)) {
		if (t.substr(i, 7) == "</dict>") {
			i += 7;
			return true;
		}
		if (!consume(t, i, "<key>")) {
			return false;
		}
		const std::string key = innerText(t, i, "</key>");
		skipWs(t, i);
		Plist value;
		if (!parseValue(t, i, value)) {
			return false;
		}
		out.dict.emplace(key, std::move(value));
	}
	return false;
}

bool parseArray(std::string_view t, size_t& i, Plist& out) {
	out.kind = Plist::Array;
	out.arr.clear();
	while (skipWs(t, i)) {
		if (t.substr(i, 8) == "</array>") {
			i += 8;
			return true;
		}
		Plist value;
		if (!parseValue(t, i, value)) {
			return false;
		}
		out.arr.push_back(std::move(value));
	}
	return false;
}

bool parseValue(std::string_view t, size_t& i, Plist& out) {
	skipWs(t, i);
	if (consume(t, i, "<dict>")) {
		return parseDict(t, i, out);
	}
	if (consume(t, i, "<array>")) {
		return parseArray(t, i, out);
	}
	if (consume(t, i, "<string>")) {
		out = Plist::str(innerText(t, i, "</string>"));
		return true;
	}
	if (consume(t, i, "<real>")) {
		out = Plist::real(std::stod(std::string(innerText(t, i, "</real>"))));
		return true;
	}
	if (consume(t, i, "<integer>")) {
		out = Plist::integer(std::stoll(std::string(innerText(t, i, "</integer>"))));
		return true;
	}
	if (consume(t, i, "<true/>") || consume(t, i, "<true />")) {
		out = Plist::integer(1);
		return true;
	}
	if (consume(t, i, "<false/>") || consume(t, i, "<false />")) {
		out = Plist::integer(0);
		return true;
	}
	return false;
}

void writeValue(std::ostringstream& o, const Plist& v, int indent);

void pad(std::ostringstream& o, int indent) {
	for (int i = 0; i < indent; ++i) {
		o << '\t';
	}
}

void writeValue(std::ostringstream& o, const Plist& v, int indent) {
	pad(o, indent);
	switch (v.kind) {
	case Plist::String:
		o << "<string>" << xmlEscape(v.s) << "</string>\n";
		break;
	case Plist::Real:
		o << "<real>" << v.n << "</real>\n";
		break;
	case Plist::Integer:
		o << "<integer>" << static_cast<long long>(v.n) << "</integer>\n";
		break;
	case Plist::Array:
		o << "<array>\n";
		for (const auto& item : v.arr) {
			writeValue(o, item, indent + 1);
		}
		pad(o, indent);
		o << "</array>\n";
		break;
	case Plist::Dict:
		o << "<dict>\n";
		for (const auto& [k, val] : v.dict) {
			pad(o, indent + 1);
			o << "<key>" << xmlEscape(k) << "</key>\n";
			writeValue(o, val, indent + 1);
		}
		pad(o, indent);
		o << "</dict>\n";
		break;
	case Plist::Null:
		o << "<string></string>\n";
		break;
	}
}

} // namespace

bool parsePlist(std::string_view xml, Plist& out) {
	auto pos = xml.find("<dict>");
	if (pos == std::string_view::npos) {
		pos = xml.find("<array>");
	}
	if (pos == std::string_view::npos) {
		return false;
	}
	size_t i = pos;
	return parseValue(xml, i, out);
}

std::string writePlist(const Plist& root) {
	std::ostringstream o;
	o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	o << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
		 "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
	o << "<plist version=\"1.0\">\n";
	writeValue(o, root, 0);
	o << "</plist>\n";
	return o.str();
}

} // namespace ufo
} // namespace rigkit
