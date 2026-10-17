#pragma once

#include <vector>
#include "ecs/PropertyReflection.h"

namespace rigkit {
namespace ecs {

/**
 * @brief Tempo and playhead authority - speaks `rig.music.transport`.
 * @details One per scene when music schemas are present. Advance in Update;
 * clock / pattern / sequencer do not own bpm or the global playhead.
 */
struct CMusicTransport {
	bool playing = false;
	float bpm = 120.f;
	int timeSigNum = 4;
	int timeSigDen = 4;
	float positionBeats = 0.f;
	bool loop = false;
	float loopStartBeats = 0.f;
	float loopEndBeats = 16.f;

	std::vector<sProp> GetProperties() {
		return {{0, "playing", EPT_BOOL, &playing},
				{1, "bpm", EPT_FLOAT, &bpm},
				{2, "timeSigNum", EPT_INT, &timeSigNum},
				{3, "timeSigDen", EPT_INT, &timeSigDen},
				{4, "positionBeats", EPT_FLOAT, &positionBeats},
				{5, "loop", EPT_BOOL, &loop},
				{6, "loopStartBeats", EPT_FLOAT, &loopStartBeats},
				{7, "loopEndBeats", EPT_FLOAT, &loopEndBeats}};
	}
};

} // namespace ecs
} // namespace rigkit
