#include "ecs/PropertyJson.h"
#include "core/TypeJson.h"

namespace rigkit {

json propsToJson(const std::vector<sProp>& props) {
	json j = json::object();
	for (const auto& prop : props) {
		if (!prop.data || prop.name.empty()) {
			continue;
		}
		nlohmann::ordered_json value;
		if (typeValueToJson(prop.type, prop.data, value)) {
			j[prop.name] = std::move(value);
		}
	}
	return j;
}

void jsonToProps(const json& j, std::vector<sProp>& props) {
	if (!j.is_object()) {
		return;
	}
	for (auto& prop : props) {
		if (!prop.data || prop.name.empty() || !j.contains(prop.name)) {
			continue;
		}
		try {
			typeValueFromJson(prop.type, prop.data, j[prop.name]);
		} catch (...) {
			// Leave field at default if the stored value is the wrong type.
		}
	}
}

} // namespace rigkit
