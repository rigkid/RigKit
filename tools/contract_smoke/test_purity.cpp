#include <doctest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef RIGKIT_SOURCE_ROOT
#error RIGKIT_SOURCE_ROOT must be defined for purity scan
#endif

namespace {

std::string toLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
				   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

bool containsForbidden(const std::string& text, const std::string& needle) {
	return toLower(text).find(toLower(needle)) != std::string::npos;
}

} // namespace

TEST_CASE("rigComponent headers stay free of UI/window code creep") {
	namespace fs = std::filesystem;
	const fs::path root = fs::path(RIGKIT_SOURCE_ROOT) / "packs" / "rigComponent" / "src";
	REQUIRE(fs::is_directory(root));

	const std::vector<std::string> forbidden = {
		"imgui.h", "imgui.hpp", "GLFW/", "glfw3.h", "std::function", "ImGui::", "GLFWwindow"};

	std::vector<std::string> hits;
	for (const auto& entry : fs::directory_iterator(root)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const auto ext = entry.path().extension().string();
		if (ext != ".h" && ext != ".hpp") {
			continue;
		}
		std::ifstream in(entry.path());
		REQUIRE(in.good());
		std::ostringstream ss;
		ss << in.rdbuf();
		const std::string body = ss.str();
		for (const auto& needle : forbidden) {
			if (containsForbidden(body, needle)) {
				hits.push_back(entry.path().filename().string() + " contains " + needle);
			}
		}
	}

	INFO(hits.size());
	for (const auto& h : hits) {
		FAIL_CHECK(h);
	}
	CHECK(hits.empty());
}
