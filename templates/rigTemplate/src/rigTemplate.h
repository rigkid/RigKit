#pragma once

#include "core/pack/IPack.h"
namespace rigkit {

/**
 * @brief Scaffold pack — rename class/files/pack.json before publishing.
 * @details Register components in setup() (data pack) or systems (code packs).
 * Prefer NO CODE JUST DATA for portable fields.
 */
class rigTemplate : public IPack {
  public:
	rigTemplate();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
