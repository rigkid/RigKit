#pragma once

#include <string>
#include <utility>
#include <vector>

namespace rigkit::json {

/// @brief One JSON value, enough for the ESP document and the preference blob.
///
/// Objects keep insertion order. Numbers are doubles. This is not a general
/// streaming parser and it does not allocate from Update/Draw.
struct Value {
	enum class Type { Null, Bool, Number, String, Array, Object };

	Type type = Type::Null;
	bool b = false;
	double n = 0;
	std::string s;
	std::vector<Value> a;
	std::vector<std::pair<std::string, Value>> o;

	static Value nul();
	static Value boolean(bool v);
	static Value number(double v);
	static Value str(std::string v);
	static Value arr();
	static Value obj();

	Value* find(const std::string& key);
	const Value* find(const std::string& key) const;
	void set(const std::string& key, Value v);

	std::string dump() const;
};

/// @brief Parse one JSON value. `err` is a short reason on failure.
bool parse(const char* text, Value& out, std::string& err);

} // namespace rigkit::json
