#include "Json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace rigkit::json {
namespace {

class Parser {
  public:
	const char* p = nullptr;
	const char* end = nullptr;
	std::string err;

	bool fail(const char* message) {
		err = message;
		return false;
	}

	void skip() {
		while (p < end) {
			const char c = *p;
			if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
				break;
			}
			++p;
		}
	}

	bool parseValue(Value& out) {
		skip();
		if (p >= end) {
			return fail("truncated json");
		}
		const char c = *p;
		if (c == '{') {
			return parseObject(out);
		}
		if (c == '[') {
			return parseArray(out);
		}
		if (c == '"') {
			out = Value::str("");
			return parseString(out.s);
		}
		if (c == 't' || c == 'f') {
			return parseBool(out);
		}
		if (c == 'n') {
			return parseNull(out);
		}
		if (c == '-' || (c >= '0' && c <= '9')) {
			return parseNumber(out);
		}
		return fail("unexpected token");
	}

	bool parseObject(Value& out) {
		++p;
		out = Value::obj();
		skip();
		if (p < end && *p == '}') {
			++p;
			return true;
		}
		while (p < end) {
			skip();
			std::string key;
			if (!parseString(key)) {
				return false;
			}
			skip();
			if (p >= end || *p != ':') {
				return fail("expected ':'");
			}
			++p;
			Value child;
			if (!parseValue(child)) {
				return false;
			}
			out.set(key, std::move(child));
			skip();
			if (p >= end) {
				return fail("truncated object");
			}
			if (*p == ',') {
				++p;
				continue;
			}
			if (*p == '}') {
				++p;
				return true;
			}
			return fail("expected ',' or '}'");
		}
		return fail("truncated object");
	}

	bool parseArray(Value& out) {
		++p;
		out = Value::arr();
		skip();
		if (p < end && *p == ']') {
			++p;
			return true;
		}
		while (p < end) {
			Value child;
			if (!parseValue(child)) {
				return false;
			}
			out.a.push_back(std::move(child));
			skip();
			if (p >= end) {
				return fail("truncated array");
			}
			if (*p == ',') {
				++p;
				continue;
			}
			if (*p == ']') {
				++p;
				return true;
			}
			return fail("expected ',' or ']'");
		}
		return fail("truncated array");
	}

	bool parseString(std::string& out) {
		skip();
		if (p >= end || *p != '"') {
			return fail("expected string");
		}
		++p;
		out.clear();
		while (p < end) {
			const unsigned char c = static_cast<unsigned char>(*p);
			++p;
			if (c == '"') {
				return true;
			}
			if (c == '\\') {
				if (p >= end) {
					return fail("truncated escape");
				}
				const char e = *p++;
				switch (e) {
				case '"':
				case '\\':
				case '/':
					out.push_back(e);
					break;
				case 'b':
					out.push_back('\b');
					break;
				case 'f':
					out.push_back('\f');
					break;
				case 'n':
					out.push_back('\n');
					break;
				case 'r':
					out.push_back('\r');
					break;
				case 't':
					out.push_back('\t');
					break;
				case 'u':
					if (!parseUnicode(out)) {
						return false;
					}
					break;
				default:
					return fail("bad escape");
				}
				continue;
			}
			if (c < 0x20) {
				return fail("control character in string");
			}
			out.push_back(static_cast<char>(c));
		}
		return fail("truncated string");
	}

	bool parseUnicode(std::string& out) {
		if (p + 4 > end) {
			return fail("truncated unicode");
		}
		int code = 0;
		for (int i = 0; i < 4; ++i) {
			const char h = *p++;
			int n = -1;
			if (h >= '0' && h <= '9') {
				n = h - '0';
			} else if (h >= 'a' && h <= 'f') {
				n = h - 'a' + 10;
			} else if (h >= 'A' && h <= 'F') {
				n = h - 'A' + 10;
			}
			if (n < 0) {
				return fail("bad unicode");
			}
			code = (code << 4) | n;
		}
		if (code >= 0xD800 && code <= 0xDFFF) {
			return fail("unicode surrogate");
		}
		if (code < 0x80) {
			out.push_back(static_cast<char>(code));
		} else if (code < 0x800) {
			out.push_back(static_cast<char>(0xC0 | (code >> 6)));
			out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
		} else {
			out.push_back(static_cast<char>(0xE0 | (code >> 12)));
			out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
		}
		return true;
	}

	bool parseBool(Value& out) {
		if (p + 4 <= end && std::memcmp(p, "true", 4) == 0) {
			p += 4;
			out = Value::boolean(true);
			return true;
		}
		if (p + 5 <= end && std::memcmp(p, "false", 5) == 0) {
			p += 5;
			out = Value::boolean(false);
			return true;
		}
		return fail("expected bool");
	}

	bool parseNull(Value& out) {
		if (p + 4 <= end && std::memcmp(p, "null", 4) == 0) {
			p += 4;
			out = Value::nul();
			return true;
		}
		return fail("expected null");
	}

	bool parseNumber(Value& out) {
		const char* start = p;
		if (*p == '-') {
			++p;
		}
		if (p >= end || *p < '0' || *p > '9') {
			return fail("bad number");
		}
		while (p < end && *p >= '0' && *p <= '9') {
			++p;
		}
		if (p < end && *p == '.') {
			++p;
			if (p >= end || *p < '0' || *p > '9') {
				return fail("bad number");
			}
			while (p < end && *p >= '0' && *p <= '9') {
				++p;
			}
		}
		if (p < end && (*p == 'e' || *p == 'E')) {
			++p;
			if (p < end && (*p == '+' || *p == '-')) {
				++p;
			}
			if (p >= end || *p < '0' || *p > '9') {
				return fail("bad number");
			}
			while (p < end && *p >= '0' && *p <= '9') {
				++p;
			}
		}
		const std::string tmp(start, p);
		char* stop = nullptr;
		const double n = std::strtod(tmp.c_str(), &stop);
		if (!stop || stop != tmp.c_str() + tmp.size()) {
			return fail("bad number");
		}
		out = Value::number(n);
		return true;
	}
};

void dumpString(const std::string& text, std::string& out) {
	out.push_back('"');
	for (unsigned char c : text) {
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\b':
			out += "\\b";
			break;
		case '\f':
			out += "\\f";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			if (c < 0x20) {
				char buf[8];
				std::snprintf(buf, sizeof buf, "\\u%04x", c);
				out += buf;
			} else {
				out.push_back(static_cast<char>(c));
			}
			break;
		}
	}
	out.push_back('"');
}

void dumpNumber(double n, std::string& out) {
	if (!std::isfinite(n)) {
		out += "0";
		return;
	}
	const double rounded = std::round(n);
	if (std::fabs(n - rounded) < 1e-9 && std::fabs(n) < 1e15) {
		char buf[32];
		std::snprintf(buf, sizeof buf, "%.0f", rounded);
		out += buf;
		return;
	}
	char buf[64];
	std::snprintf(buf, sizeof buf, "%.8g", n);
	out += buf;
}

void dumpValue(const Value& v, std::string& out) {
	switch (v.type) {
	case Value::Type::Null:
		out += "null";
		break;
	case Value::Type::Bool:
		out += v.b ? "true" : "false";
		break;
	case Value::Type::Number:
		dumpNumber(v.n, out);
		break;
	case Value::Type::String:
		dumpString(v.s, out);
		break;
	case Value::Type::Array:
		out.push_back('[');
		for (size_t i = 0; i < v.a.size(); ++i) {
			if (i) {
				out.push_back(',');
			}
			dumpValue(v.a[i], out);
		}
		out.push_back(']');
		break;
	case Value::Type::Object:
		out.push_back('{');
		for (size_t i = 0; i < v.o.size(); ++i) {
			if (i) {
				out.push_back(',');
			}
			dumpString(v.o[i].first, out);
			out.push_back(':');
			dumpValue(v.o[i].second, out);
		}
		out.push_back('}');
		break;
	}
}

} // namespace

Value Value::nul() {
	return {};
}

Value Value::boolean(bool v) {
	Value x;
	x.type = Type::Bool;
	x.b = v;
	return x;
}

Value Value::number(double v) {
	Value x;
	x.type = Type::Number;
	x.n = v;
	return x;
}

Value Value::str(std::string v) {
	Value x;
	x.type = Type::String;
	x.s = std::move(v);
	return x;
}

Value Value::arr() {
	Value x;
	x.type = Type::Array;
	return x;
}

Value Value::obj() {
	Value x;
	x.type = Type::Object;
	return x;
}

Value* Value::find(const std::string& key) {
	if (type != Type::Object) {
		return nullptr;
	}
	for (auto& kv : o) {
		if (kv.first == key) {
			return &kv.second;
		}
	}
	return nullptr;
}

const Value* Value::find(const std::string& key) const {
	if (type != Type::Object) {
		return nullptr;
	}
	for (const auto& kv : o) {
		if (kv.first == key) {
			return &kv.second;
		}
	}
	return nullptr;
}

void Value::set(const std::string& key, Value v) {
	if (type != Type::Object) {
		type = Type::Object;
		o.clear();
	}
	if (Value* cur = find(key)) {
		*cur = std::move(v);
		return;
	}
	o.emplace_back(key, std::move(v));
}

std::string Value::dump() const {
	std::string out;
	out.reserve(256);
	dumpValue(*this, out);
	return out;
}

bool parse(const char* text, Value& out, std::string& err) {
	if (!text) {
		err = "empty json";
		return false;
	}
	Parser parser;
	parser.p = text;
	parser.end = text + std::strlen(text);
	if (parser.p + 3 <= parser.end && static_cast<unsigned char>(parser.p[0]) == 0xEF) {
		parser.p += 3;
	}
	if (!parser.parseValue(out)) {
		err = parser.err.empty() ? "bad json" : parser.err;
		return false;
	}
	parser.skip();
	if (parser.p != parser.end) {
		err = "trailing data";
		return false;
	}
	err.clear();
	return true;
}

} // namespace rigkit::json
