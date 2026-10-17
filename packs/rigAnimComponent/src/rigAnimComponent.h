#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Tween, LFO, and binding PODs. Data only.
 */
class rigAnimComponent : public IPack {
  public:
	rigAnimComponent();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
