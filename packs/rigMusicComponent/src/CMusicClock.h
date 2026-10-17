#pragma once

#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Tick derivation from transport or external sync - speaks `rig.music.clock`.
 * @details Does not own bpm; read `CMusicTransport`. External writers (Link, etc.)
 * set `externalSync` and fill `syncBeat` / `syncPhase` each Update.
 */
struct CMusicClock {
	int ticksPerQuarter = 96;
	float phaseTicks = 0.f;
	float swingAmount = 0.f;
	int swingSubdiv = 2;
	bool externalSync = false;
	float syncBeat = 0.f;
	float syncPhase = 0.f;
	float syncPeriodBars = 0.f;

	std::vector<sProp> GetProperties() {
		return {{0, "ticksPerQuarter", EPT_INT, &ticksPerQuarter},
				{1, "phaseTicks", EPT_FLOAT, &phaseTicks},
				{2, "swingAmount", EPT_FLOAT, &swingAmount},
				{3, "swingSubdiv", EPT_INT, &swingSubdiv},
				{4, "externalSync", EPT_BOOL, &externalSync},
				{5, "syncBeat", EPT_FLOAT, &syncBeat},
				{6, "syncPhase", EPT_FLOAT, &syncPhase},
				{7, "syncPeriodBars", EPT_FLOAT, &syncPeriodBars}};
	}
};

} // namespace ecs
} // namespace rigkit
