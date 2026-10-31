#pragma once
#include "core/json.h"

namespace rigkit {

/**
 * @brief JSON get/set blob on engine, packs, and managers.
 * @details User preference sections live on MSettings, not here.
 * @see docs/contract/seams.md, docs/settings.md
 */
class ISettings {
  public:
	virtual ~ISettings() = default;
	virtual json getSettings() const = 0;
	virtual void setSettings(const json& settings) = 0;
};

} // namespace rigkit
