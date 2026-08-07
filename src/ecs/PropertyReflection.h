#pragma once
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

// Property type enumeration for editor reflection.
// Maps to the Rig Contract's portable datatype ids
// (docs/contract/RigWorks/docs/properties.md#datatype-ids).
enum propTypes {
	EPT_BOOL = 1,
	EPT_INT,
	EPT_ENUM, // Named choice over an int; always pairs with enumNames/enumCount.
	EPT_UINT,
	EPT_FLOAT,
	EPT_DOUBLE,
	EPT_STRING,
	EPT_VEC2, // glm::vec2 / float[2] — UI packs may map to ImVec2
	EPT_VEC3, // glm::vec3 / float[3]
	EPT_VEC4, // glm::vec4 / float[4] — UI packs may map to ImVec4
	/// Colour as float[4]. Host extension, not a Contract datatype id — the
	/// Contract keeps colour as field-name convention over `vec4` (properties.md
	/// says "hosts may add prefixed ids"); this is that extension for rigImGui's
	/// colour picker.
	EPT_COLOR,
	// Legacy ImGui-named aliases (same values)
	EPT_IMVEC4 = EPT_VEC4,
	EPT_IMVEC2 = EPT_VEC2,
	EPT_COUNT
};

// Property descriptor struct for editor reflection
struct sProp {
	uint32_t id;
	std::string name;
	propTypes type;
	void* data;
	/// Combo labels — required for EPT_ENUM (editors show a "misconfigured" state
	/// without them rather than falling back to a raw draggable int). Lifetime
	/// must outlive the editor draw.
	const char* const* enumNames = nullptr;
	int enumCount = 0;
};

// SFINAE: Checks if T has GetProperties() returning std::vector<sProp>
template <typename T> class has_get_properties {
  private:
	template <typename U>
	static auto test(int) -> decltype(std::declval<U>().GetProperties(), std::true_type{});
	template <typename> static std::false_type test(...);

  public:
	static constexpr bool value = decltype(test<T>(0))::value;
};

// If T has GetProperties(), call it
template <typename T>
typename std::enable_if<has_get_properties<T>::value, std::vector<sProp>>::type TryGetProperties(
	T& component) {
	return component.GetProperties();
}

// If T does not have GetProperties(), return empty and log warning
template <typename T>
typename std::enable_if<!has_get_properties<T>::value, std::vector<sProp>>::type TryGetProperties(
	T&) {
	spdlog::warn("[Property Inspector] WARNING: Component '{}' missing GetProperties()",
				 typeid(T).name());
	return {};
}
