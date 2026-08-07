#pragma once
#include "core/json.h" // Or wherever your JSON type is defined

class ISettings {
  public:
	virtual ~ISettings() = default;
	virtual rigkit::json getSettings() const = 0;
	virtual void setSettings(const rigkit::json& settings) = 0;
};
