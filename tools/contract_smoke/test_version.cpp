#include <doctest.h>

#include <regex>
#include "core/RigKitVersion.h"

TEST_CASE("rigkit::version is host SemVer") {
	const char* v = rigkit::version();
	REQUIRE(v != nullptr);
	REQUIRE(v[0] != '\0');
	CHECK(std::regex_match(v, std::regex("^[0-9]+\\.[0-9]+\\.[0-9]+$")));
	CHECK(rigkit::versionMajor() >= 0);
	CHECK(rigkit::versionMinor() >= 0);
	CHECK(rigkit::versionPatch() >= 0);
}

