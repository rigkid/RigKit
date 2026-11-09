#pragma once

#include "App.h"

#include <string>

namespace rigkit::espapp {

const char* radioModeName(RadioMode mode);
bool radioModeParse(const char* text, RadioMode& mode);
bool radioMerge(Radio& radio, const std::string& json, std::string& err);
std::string radioWrite(const Radio& radio);
std::string prefsResponse(const Radio& radio);

} // namespace rigkit::espapp
