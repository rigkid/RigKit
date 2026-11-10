#pragma once

#include <cstddef>
#include <cstdint>

namespace rigkit::espapp {

/// @brief Push interleaved 16-bit stereo. Dropped when I2S is not in this build.
void pcmWrite(const int16_t* interleavedStereo, size_t frames);

/// @brief Pop up to `frames` interleaved stereo frames. Returns how many were copied.
size_t pcmRead(int16_t* interleavedStereo, size_t frames);

} // namespace rigkit::espapp
