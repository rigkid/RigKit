#pragma once

#include <string>
#include <vector>

#include "ecs/PropertyReflection.h"

namespace stage {

/**
 * @brief Play script path, relative to `<exeDir>/data` unless absolute.
 * @details Source stays a file so an agent can edit it. The app runs `update(dt)`
 * each Update. `enabled` false skips the file.
 */
struct CStageScript {
	std::string path = "play.py";
	bool enabled = true;

	std::vector<sProp> GetProperties() {
		return {{0, "Path", EPT_STRING, &path}, {1, "Enabled", EPT_BOOL, &enabled}};
	}
};

} // namespace stage
