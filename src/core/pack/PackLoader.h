#pragma once

#include <memory>

#include "IPack.h"
#include "MPack.h"

namespace rigkit {

/**
 * Centralized pack loading utility
 *
 * This class provides a clean interface for applications to register
 * and load packs without relying on static initialization or
 * individual Registration.cpp files.
 */
class PackLoader {
  public:
	/**
	 * Register all available packs with the pack manager
	 *
	 * @param packManager The MPack instance to register packs with
	 * @param packNames List of pack names to register (empty = all available)
	 */
	static void registerAvailablePacks(MPack* packManager,
									   const std::vector<std::string>& packNames = {});

	/**
	 * Register a specific pack by name
	 *
	 * @param packManager The MPack instance to register the pack with
	 * @param packName The name of the pack to register
	 * @return true if the pack was successfully registered
	 */
	static bool registerPack(MPack* packManager, const std::string& packName);

	/**
	 * Get a list of all available pack names
	 */
	static std::vector<std::string> getAvailablePackNames();
};

} // namespace rigkit
