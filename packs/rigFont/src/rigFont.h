#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Shape / reconstruct over `rig.font.*` source PODs. Code only.
 */
class rigFont : public IPack {
  public:
	rigFont();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
