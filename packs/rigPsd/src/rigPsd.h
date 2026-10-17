#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Leaf PSD IO pack - format model only, no document engine.
 */
class rigPsd : public IPack {
  public:
	rigPsd();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
