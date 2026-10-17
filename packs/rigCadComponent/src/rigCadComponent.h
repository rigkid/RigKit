#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief CAD solid and datum PODs (`CCad*`). Data only.
 */
class rigCadComponent : public IPack {
  public:
	rigCadComponent();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
