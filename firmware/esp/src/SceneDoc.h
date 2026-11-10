#pragma once

#include "App.h"

#include <string>

namespace rigkit::espapp {

bool sceneLoadDefault(App& app, std::string& err);
bool sceneRead(const std::string& json, App& app, std::string& err);
std::string scenePublish(const App& app);
bool sceneOverlayRead(const std::string& json, App& app);
std::string sceneOverlayWrite(const App& app);

} // namespace rigkit::espapp
