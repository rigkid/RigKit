#pragma once

#include <string>

namespace rigkit {
class MEcs;
class rigPython;
} // namespace rigkit

namespace stage {

/**
 * @brief Inject the `stage` module into the embedded interpreter.
 * @details Needs CPython embed (`rigPython::available`). Functions read and write
 * @p ecs. See `data/play.py` for the call set.
 */
bool install(rigkit::rigPython& py, rigkit::MEcs* ecs, std::string* err = nullptr);

/**
 * @brief Run every enabled `CStageScript`: load the file when it changes, then
 * call `update(dt)` if the script defined it.
 * @return false on a load or `update` error (@p err set).
 */
bool runScripts(rigkit::MEcs& ecs, rigkit::rigPython& py, float dt, std::string* err = nullptr);

} // namespace stage
