#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "CFontComponent.h"
#include "CPath.h"

namespace rigkit {
namespace ufo {

struct GlifAnchor {
	std::string name;
	float x = 0.f;
	float y = 0.f;
};

struct Glif {
	std::string name;
	float width = 0.f;
	std::vector<uint32_t> unicodes;
	rigkit::ecs::CPath path;
	std::vector<rigkit::ecs::CFontComponent> components;
	std::vector<GlifAnchor> anchors;
};

bool parseGlif(std::string_view xml, Glif& out);
std::string writeGlif(const Glif& g);

} // namespace ufo
} // namespace rigkit
