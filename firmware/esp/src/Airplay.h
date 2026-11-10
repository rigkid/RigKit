#pragma once

#include "App.h"

namespace rigkit::espapp {

void airplayApply(bool enabled, const char* name);
void airplayRead(AudioCache& out);

} // namespace rigkit::espapp
