#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Music clock and transport PODs. Data only.
 */
class rigMusicComponent : public IPack {
  public:
	rigMusicComponent();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
