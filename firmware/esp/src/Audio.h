#pragma once

#include "App.h"

namespace rigkit::espapp {

void audioApply(bool bluetooth, const char* btName, bool airplay, const char* airName);
void audioPump();
void audioRead(AudioCache& out);

} // namespace rigkit::espapp
