#include "core/util/CommandLineArgs.h"
#include <algorithm>
#include <iostream>

namespace rigkit {

CommandLineArgs::CommandLineArgs(int argc, char* argv[]) {
	// Store all arguments
	for (int i = 0; i < argc; ++i) {
		m_allArgs.push_back(argv[i]);
	}

	// Parse arguments
	for (int i = 1; i < argc; ++i) { // Skip argv[0] (program name)
		std::string arg = argv[i];

		if (arg.empty())
			continue;

		if (arg[0] == '-') {
			// Handle flags and parameters
			if (arg.length() > 1 && arg[1] == '-') {
				// Long form: --flag, --param=value, or --param value
				std::string param = arg.substr(2);
				size_t equalPos = param.find('=');

				if (equalPos != std::string::npos) {
					std::string key = param.substr(0, equalPos);
					std::string value = param.substr(equalPos + 1);
					m_params[key] = value;
				} else if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
					m_params[param] = argv[++i];
				} else {
					m_flags[param] = true;
				}
			} else {
				// Short form: -f, -f=value, or -f value
				std::string param = arg.substr(1);
				size_t equalPos = param.find('=');

				if (equalPos != std::string::npos) {
					std::string key = param.substr(0, equalPos);
					std::string value = param.substr(equalPos + 1);
					m_params[key] = value;
				} else if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
					m_params[param] = argv[++i];
				} else {
					m_flags[param] = true;
				}
			}
		} else {
			// Positional argument
			m_positionalArgs.push_back(arg);
		}
	}
}

bool CommandLineArgs::hasFlag(const std::string& flag) const {
	return m_flags.find(flag) != m_flags.end();
}

std::optional<std::string> CommandLineArgs::getValue(const std::string& param) const {
	auto it = m_params.find(param);
	if (it != m_params.end()) {
		return it->second;
	}
	return std::nullopt;
}

std::string CommandLineArgs::getValue(const std::string& param,
									  const std::string& defaultValue) const {
	auto value = getValue(param);
	return value.value_or(defaultValue);
}

void CommandLineArgs::printUsage(const std::string& appName,
								 const std::vector<std::string>& options) const {
	std::cout << "Usage: " << appName;
	if (!options.empty()) {
		std::cout << " [options]";
	}
	std::cout << std::endl;

	if (!options.empty()) {
		std::cout << "\nOptions:" << std::endl;
		for (const auto& option : options) {
			std::cout << "  " << option << std::endl;
		}
	}

	std::cout << "\nExamples:" << std::endl;
	std::cout << "  " << appName << " --config=settings.json" << std::endl;
	std::cout << "  " << appName << " --debug --app-id=myapp" << std::endl;
	std::cout << "  " << appName << " --window-size=1920x1080" << std::endl;
}

} // namespace rigkit
