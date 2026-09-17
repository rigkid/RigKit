#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace rigkit {
namespace ufo {

struct Plist {
	enum Kind { Null, String, Real, Integer, Array, Dict };

	Kind kind = Null;
	std::string s;
	double n = 0;
	std::vector<Plist> arr;
	std::map<std::string, Plist> dict;

	static Plist str(std::string v) {
		Plist p;
		p.kind = String;
		p.s = std::move(v);
		return p;
	}
	static Plist real(double v) {
		Plist p;
		p.kind = Real;
		p.n = v;
		return p;
	}
	static Plist integer(long long v) {
		Plist p;
		p.kind = Integer;
		p.n = static_cast<double>(v);
		return p;
	}

	const Plist* get(const char* key) const {
		auto it = dict.find(key);
		return it == dict.end() ? nullptr : &it->second;
	}
	std::string string(const char* key, const std::string& fallback = {}) const {
		const auto* v = get(key);
		return (v && v->kind == String) ? v->s : fallback;
	}
	double number(const char* key, double fallback = 0) const {
		const auto* v = get(key);
		if (!v || (v->kind != Real && v->kind != Integer)) {
			return fallback;
		}
		return v->n;
	}
};

bool parsePlist(std::string_view xml, Plist& out);
std::string writePlist(const Plist& root);

} // namespace ufo
} // namespace rigkit
