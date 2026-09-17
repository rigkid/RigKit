#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief UFO source PODs (`rig.font.*`). Data only.
 */
class rigFontComponent : public IPack {
  public:
	rigFontComponent();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
