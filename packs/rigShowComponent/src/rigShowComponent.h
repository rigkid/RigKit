#pragma once

#include "core/pack/IPack.h"

namespace rigkit {

/**
 * @brief Show deck + analysis PODs (`CVisualizer` / `CAudioAnalysis` / `CPictureAnalysis`).
 */
class rigShowComponent : public IPack {
  public:
	rigShowComponent();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
