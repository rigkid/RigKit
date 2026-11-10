#pragma once

#include "App.h"

namespace rigkit::espapp {

void a2dpApply(bool enabled, const char* name);
void a2dpRead(AudioCache& out);

} // namespace rigkit::espapp
