#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief `.ufo` / `.ufoz` read/write into `rig.font.*` PODs.
 */
class rigUfo : public IPack {
  public:
	rigUfo();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
