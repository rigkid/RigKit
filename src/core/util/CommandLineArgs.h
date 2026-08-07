#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rigkit {

class CommandLineArgs {
  public:
	CommandLineArgs() = default;
	CommandLineArgs(int argc, char* argv[]);

	// Check if a flag is present
	bool hasFlag(const std::string& flag) const;

	// Get value for a parameter (e.g., --config=file.json)
	std::optional<std::string> getValue(const std::string& param) const;

	// Get value with default
	std::string getValue(const std::string& param, const std::string& defaultValue) const;

	// Get positional arguments (non-flag arguments)
	const std::vector<std::string>& getPositionalArgs() const { return m_positionalArgs; }

	// Get all arguments for debugging
	const std::vector<std::string>& getAllArgs() const { return m_allArgs; }

	// Print usage help
	void printUsage(const std::string& appName, const std::vector<std::string>& options = {}) const;

  private:
	std::vector<std::string> m_allArgs;
	std::vector<std::string> m_positionalArgs;
	std::unordered_map<std::string, std::string> m_params;
	std::unordered_map<std::string, bool> m_flags;
};

} // namespace rigkit
