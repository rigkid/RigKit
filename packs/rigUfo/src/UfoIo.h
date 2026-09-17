#pragma once

#include <entt/entt.hpp>
#include <string>

#include "ecs/MEcs.h"

namespace rigkit {
namespace ufo {

struct UfoResult {
	bool ok = false;
	std::string error;
	entt::entity face{entt::null};
};

/**
 * @brief Read a `.ufo` directory or `.ufoz` zip into `rig.font.*` + `CPath`.
 * @details Replaces existing font entities. Does not touch unrelated ECS data.
 */
UfoResult loadUfo(MEcs& ecs, const std::string& path);

/**
 * @brief Write the first `CFontFace` and its children as `.ufo` or `.ufoz`.
 */
bool saveUfo(MEcs& ecs, const std::string& path, std::string* error = nullptr);

/** @brief True when `fontmake` is on PATH (desktop export). */
bool fontmakeAvailable();

/**
 * @brief Save a temp `.ufo` and run fontmake to a static `.ttf`.
 * @details Missing fontmake returns false with an error. Never the live source.
 */
bool exportTtf(MEcs& ecs, const std::string& outPath, std::string* error = nullptr);

} // namespace ufo
} // namespace rigkit
