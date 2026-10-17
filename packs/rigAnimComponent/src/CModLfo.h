#pragma once

#include <string>
#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Low-frequency oscillator - speaks `rig.mod.lfo`.
 * @details `lastSample` is a runtime cache (not serialized). Drive targets through
 * `CModBinding` + `SModulators` (Update), not a free-function tick.
 */
struct CModLfo {
	std::string waveform = "sine"; ///< sine | tri | saw | square
	float frequency = 1.f;		   ///< Hz
	float amplitude = 1.f;
	float offset = 0.f;
	float phase = 0.f; ///< 0-1 wrap
	float lastSample = 0.f;

	std::vector<sProp> GetProperties() {
		return {{0, "frequency", EPT_FLOAT, &frequency},
				{1, "amplitude", EPT_FLOAT, &amplitude},
				{2, "offset", EPT_FLOAT, &offset},
				{3, "phase", EPT_FLOAT, &phase},
				{4, "waveform", EPT_STRING, &waveform}};
	}
};

} // namespace ecs
} // namespace rigkit
