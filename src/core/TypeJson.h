#pragma once

/**
 * @file TypeJson.h
 * @brief Contract property datatypes  <->  nlohmann JSON.
 * @details One include for host + pack codecs: scalars, vec2/3/4, quat, colour
 * (host `EPT_COLOR` over vec4), and `typeValueToJson` / `typeValueFromJson` for
 * `sProp` / `propTypes`. Quat wire order is x, y, z, w (glm ctor is w, x, y, z).
 * `*ToJson` returns `ordered_json` (assigns into `json` too); `*FromJson`
 * accepts either. Colour arrays may be rgb or rgba (`rgbaFromJson`).
 *
 * Contract table: docs/contract/RigWorks/docs/properties.md - `curve` stays
 * schema-shaped (not a leaf helper here).
 */

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include "ecs/PropertyReflection.h"

namespace rigkit {

// --- scalars ---------------------------------------------------------------

inline nlohmann::ordered_json boolToJson(bool v) {
	return v;
}
template <typename Json> bool boolFromJson(const Json& j, bool fallback = false) {
	if (!j.is_boolean()) {
		return fallback;
	}
	return j.template get<bool>();
}

inline nlohmann::ordered_json intToJson(int v) {
	return v;
}
template <typename Json> int intFromJson(const Json& j, int fallback = 0) {
	if (!j.is_number_integer() && !j.is_number_unsigned()) {
		return fallback;
	}
	return j.template get<int>();
}

inline nlohmann::ordered_json uintToJson(unsigned int v) {
	return v;
}
template <typename Json> unsigned int uintFromJson(const Json& j, unsigned int fallback = 0) {
	if (!j.is_number_integer() && !j.is_number_unsigned()) {
		return fallback;
	}
	return j.template get<unsigned int>();
}

inline nlohmann::ordered_json floatToJson(float v) {
	return v;
}
template <typename Json> float floatFromJson(const Json& j, float fallback = 0.f) {
	if (!j.is_number()) {
		return fallback;
	}
	return j.template get<float>();
}

inline nlohmann::ordered_json doubleToJson(double v) {
	return v;
}
template <typename Json> double doubleFromJson(const Json& j, double fallback = 0.0) {
	if (!j.is_number()) {
		return fallback;
	}
	return j.template get<double>();
}

inline nlohmann::ordered_json stringToJson(const std::string& v) {
	return v;
}
template <typename Json>
std::string stringFromJson(const Json& j, const std::string& fallback = {}) {
	if (!j.is_string()) {
		return fallback;
	}
	return j.template get<std::string>();
}

/// Named choice - same wire as int (schema lists literals).
inline nlohmann::ordered_json enumToJson(int v) {
	return intToJson(v);
}
template <typename Json> int enumFromJson(const Json& j, int fallback = 0) {
	return intFromJson(j, fallback);
}

/// Entity id as host integer (Contract `entity`).
inline nlohmann::ordered_json entityToJson(std::uint32_t v) {
	return v;
}
template <typename Json> std::uint32_t entityFromJson(const Json& j, std::uint32_t fallback = 0) {
	if (!j.is_number_integer() && !j.is_number_unsigned()) {
		return fallback;
	}
	return j.template get<std::uint32_t>();
}

// --- vectors / quat / colour -----------------------------------------------

inline nlohmann::ordered_json vec2ToJson(const glm::vec2& v) {
	return nlohmann::ordered_json::array({v.x, v.y});
}
template <typename Json> glm::vec2 vec2FromJson(const Json& j, const glm::vec2& fallback = {}) {
	if (!j.is_array() || j.size() < 2) {
		return fallback;
	}
	return {j[0].template get<float>(), j[1].template get<float>()};
}

inline nlohmann::ordered_json vec3ToJson(const glm::vec3& v) {
	return nlohmann::ordered_json::array({v.x, v.y, v.z});
}
template <typename Json> glm::vec3 vec3FromJson(const Json& j, const glm::vec3& fallback = {}) {
	if (!j.is_array() || j.size() < 3) {
		return fallback;
	}
	return {j[0].template get<float>(), j[1].template get<float>(), j[2].template get<float>()};
}

inline nlohmann::ordered_json vec4ToJson(const glm::vec4& v) {
	return nlohmann::ordered_json::array({v.x, v.y, v.z, v.w});
}
template <typename Json> glm::vec4 vec4FromJson(const Json& j, const glm::vec4& fallback = {}) {
	if (!j.is_array() || j.size() < 4) {
		return fallback;
	}
	return {j[0].template get<float>(), j[1].template get<float>(), j[2].template get<float>(),
			j[3].template get<float>()};
}

/// rgb or rgba array to vec4 (A defaults to `fallback.a`, usually 1).
template <typename Json>
glm::vec4 rgbaFromJson(const Json& j, const glm::vec4& fallback = {0.f, 0.f, 0.f, 1.f}) {
	if (!j.is_array() || j.size() < 3) {
		return fallback;
	}
	return {j[0].template get<float>(), j[1].template get<float>(), j[2].template get<float>(),
			j.size() > 3 ? j[3].template get<float>() : fallback.a};
}

/// Colour as float[4]. Missing alpha to fallback A (typically 1).
inline nlohmann::ordered_json colorToJson(const glm::vec4& v) {
	return vec4ToJson(v);
}
template <typename Json>
glm::vec4 colorFromJson(const Json& j, const glm::vec4& fallback = {0.f, 0.f, 0.f, 1.f}) {
	return rgbaFromJson(j, fallback);
}

/// rgb array (no alpha) - light colour, emissive, etc.
inline nlohmann::ordered_json rgbToJson(const glm::vec3& v) {
	return vec3ToJson(v);
}
template <typename Json> glm::vec3 rgbFromJson(const Json& j, const glm::vec3& fallback = {}) {
	return vec3FromJson(j, fallback);
}

/// Rig quat field order: x, y, z, w.
inline nlohmann::ordered_json quatToJson(const glm::quat& q) {
	return nlohmann::ordered_json::array({q.x, q.y, q.z, q.w});
}
template <typename Json>
glm::quat quatFromJson(const Json& j, const glm::quat& fallback = glm::quat(1.f, 0.f, 0.f, 0.f)) {
	if (!j.is_array() || j.size() < 4) {
		return fallback;
	}
	return glm::normalize(glm::quat(j[3].template get<float>(), j[0].template get<float>(),
									j[1].template get<float>(), j[2].template get<float>()));
}

// --- propTypes dispatch (GetProperties / settings) -------------------------

/**
 * @brief Write one property value into `out`.
 * @return false if type is unsupported or `data` is null (caller skips the key).
 */
inline bool typeValueToJson(propTypes type, const void* data, nlohmann::ordered_json& out) {
	if (!data) {
		return false;
	}
	switch (type) {
	case EPT_BOOL:
		out = boolToJson(*static_cast<const bool*>(data));
		return true;
	case EPT_INT:
	case EPT_ENUM:
		out = intToJson(*static_cast<const int*>(data));
		return true;
	case EPT_UINT:
		out = uintToJson(*static_cast<const unsigned int*>(data));
		return true;
	case EPT_FLOAT:
		out = floatToJson(*static_cast<const float*>(data));
		return true;
	case EPT_DOUBLE:
		out = doubleToJson(*static_cast<const double*>(data));
		return true;
	case EPT_STRING:
		out = stringToJson(*static_cast<const std::string*>(data));
		return true;
	case EPT_VEC2:
		out = vec2ToJson(*static_cast<const glm::vec2*>(data));
		return true;
	case EPT_VEC3:
		out = vec3ToJson(*static_cast<const glm::vec3*>(data));
		return true;
	case EPT_VEC4:
	case EPT_COLOR:
		out = vec4ToJson(*static_cast<const glm::vec4*>(data));
		return true;
	default:
		return false;
	}
}

/**
 * @brief Read one property value from `j` into `data`.
 * @return false if type unsupported, null data, or wrong JSON shape (leave field).
 */
template <typename Json> bool typeValueFromJson(propTypes type, void* data, const Json& j) {
	if (!data) {
		return false;
	}
	switch (type) {
	case EPT_BOOL:
		if (!j.is_boolean()) {
			return false;
		}
		*static_cast<bool*>(data) = boolFromJson(j, *static_cast<bool*>(data));
		return true;
	case EPT_INT:
	case EPT_ENUM:
		if (!j.is_number_integer() && !j.is_number_unsigned()) {
			return false;
		}
		*static_cast<int*>(data) = intFromJson(j, *static_cast<int*>(data));
		return true;
	case EPT_UINT:
		if (!j.is_number_integer() && !j.is_number_unsigned()) {
			return false;
		}
		*static_cast<unsigned int*>(data) = uintFromJson(j, *static_cast<unsigned int*>(data));
		return true;
	case EPT_FLOAT:
		if (!j.is_number()) {
			return false;
		}
		*static_cast<float*>(data) = floatFromJson(j, *static_cast<float*>(data));
		return true;
	case EPT_DOUBLE:
		if (!j.is_number()) {
			return false;
		}
		*static_cast<double*>(data) = doubleFromJson(j, *static_cast<double*>(data));
		return true;
	case EPT_STRING:
		if (!j.is_string()) {
			return false;
		}
		*static_cast<std::string*>(data) = stringFromJson(j, *static_cast<std::string*>(data));
		return true;
	case EPT_VEC2: {
		if (!j.is_array() || j.size() < 2) {
			return false;
		}
		auto* dst = static_cast<glm::vec2*>(data);
		*dst = vec2FromJson(j, *dst);
		return true;
	}
	case EPT_VEC3: {
		if (!j.is_array() || j.size() < 3) {
			return false;
		}
		auto* dst = static_cast<glm::vec3*>(data);
		*dst = vec3FromJson(j, *dst);
		return true;
	}
	case EPT_VEC4:
	case EPT_COLOR: {
		if (!j.is_array() || j.size() < 4) {
			return false;
		}
		auto* dst = static_cast<glm::vec4*>(data);
		*dst = vec4FromJson(j, *dst);
		return true;
	}
	default:
		return false;
	}
}

} // namespace rigkit
