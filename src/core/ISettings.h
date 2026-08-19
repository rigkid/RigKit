#pragma once
#include "core/json.h"

class ISettings {
  public:
	virtual ~ISettings() = default;
	virtual rigkit::json getSettings() const = 0;
	virtual void setSettings(const rigkit::json& settings) = 0;
};
