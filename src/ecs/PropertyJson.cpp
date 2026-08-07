#include "ecs/PropertyJson.h"

#include <glm/glm.hpp>
#include <string>

namespace rigkit {

json propsToJson(const std::vector<sProp>& props) {
	json j = json::object();
	for (const auto& prop : props) {
		if (!prop.data || prop.name.empty()) {
			continue;
		}
		switch (prop.type) {
		case EPT_BOOL:
			j[prop.name] = *static_cast<const bool*>(prop.data);
			break;
		case EPT_INT:
		case EPT_ENUM:
			j[prop.name] = *static_cast<const int*>(prop.data);
			break;
		case EPT_UINT:
			j[prop.name] = *static_cast<const unsigned int*>(prop.data);
			break;
		case EPT_FLOAT:
			j[prop.name] = *static_cast<const float*>(prop.data);
			break;
		case EPT_DOUBLE:
			j[prop.name] = *static_cast<const double*>(prop.data);
			break;
		case EPT_STRING:
			j[prop.name] = *static_cast<const std::string*>(prop.data);
			break;
		case EPT_VEC2: {
			const auto* v = static_cast<const glm::vec2*>(prop.data);
			j[prop.name] = {v->x, v->y};
			break;
		}
		case EPT_VEC3: {
			const auto* v = static_cast<const glm::vec3*>(prop.data);
			j[prop.name] = {v->x, v->y, v->z};
			break;
		}
		case EPT_VEC4:
		case EPT_COLOR: {
			const auto* v = static_cast<const glm::vec4*>(prop.data);
			j[prop.name] = {v->x, v->y, v->z, v->w};
			break;
		}
		default:
			break;
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
		const auto& v = j[prop.name];
		try {
			switch (prop.type) {
			case EPT_BOOL:
				*static_cast<bool*>(prop.data) = v.get<bool>();
				break;
			case EPT_INT:
			case EPT_ENUM:
				*static_cast<int*>(prop.data) = v.get<int>();
				break;
			case EPT_UINT:
				*static_cast<unsigned int*>(prop.data) = v.get<unsigned int>();
				break;
			case EPT_FLOAT:
				*static_cast<float*>(prop.data) = v.get<float>();
				break;
			case EPT_DOUBLE:
				*static_cast<double*>(prop.data) = v.get<double>();
				break;
			case EPT_STRING:
				*static_cast<std::string*>(prop.data) = v.get<std::string>();
				break;
			case EPT_VEC2:
				if (v.is_array() && v.size() >= 2) {
					*static_cast<glm::vec2*>(prop.data) =
						glm::vec2(v[0].get<float>(), v[1].get<float>());
				}
				break;
			case EPT_VEC3:
				if (v.is_array() && v.size() >= 3) {
					*static_cast<glm::vec3*>(prop.data) =
						glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
				}
				break;
			case EPT_VEC4:
			case EPT_COLOR:
				if (v.is_array() && v.size() >= 4) {
					*static_cast<glm::vec4*>(prop.data) = glm::vec4(
						v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>());
				}
				break;
			default:
				break;
			}
		} catch (...) {
			// Leave field at default if the stored value is the wrong type.
		}
	}
}

} // namespace rigkit
