#pragma once

namespace rigkit {

/**
 * @brief Host SemVer from `cmake/VERSION` (CMake project version).
 * @details App and pack versions stay in `app.json` / `pack.json`. Do not copy
 * this string into docs — bump `cmake/VERSION` and add a History row.
 * @return Pointer to a static `MAJOR.MINOR.PATCH` literal.
 * @see docs/versioning.md
 */
const char* version();

/** @brief Host SemVer major. */
int versionMajor();
/** @brief Host SemVer minor. */
int versionMinor();
/** @brief Host SemVer patch. */
int versionPatch();

} // namespace rigkit
