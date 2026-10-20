#include "CFontMaster.h"

namespace rigkit {
namespace ecs {

float CFontMaster::valueFor(const std::string& tag, float fallback) const {
	for (const auto& av : location) {
		if (av.tag == tag) {
			return av.value;
		}
	}
	return fallback;
}

void CFontMaster::setValue(const std::string& tag, float value) {
	for (auto& av : location) {
		if (av.tag == tag) {
			av.value = value;
			return;
		}
	}
	location.push_back({tag, value});
}

} // namespace ecs
} // namespace rigkit
