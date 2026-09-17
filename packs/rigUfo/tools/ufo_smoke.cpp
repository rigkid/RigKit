#include "Glif.h"
#include "Plist.h"

#include <cmath>
#include <cstdio>

int main() {
	rigkit::ufo::Plist info;
	info.kind = rigkit::ufo::Plist::Dict;
	info.dict["familyName"] = rigkit::ufo::Plist::str("LiveFace");
	info.dict["unitsPerEm"] = rigkit::ufo::Plist::integer(1000);
	const auto xml = rigkit::ufo::writePlist(info);
	rigkit::ufo::Plist back;
	if (!rigkit::ufo::parsePlist(xml, back) || back.string("familyName") != "LiveFace") {
		std::fprintf(stderr, "plist roundtrip failed\n");
		return 1;
	}

	rigkit::ufo::Glif g;
	g.name = "A";
	g.width = 600.f;
	g.unicodes = {65};
	using Cmd = rigkit::ecs::CPath::Cmd;
	g.path.commands.push_back({Cmd::MoveTo, {50.f, 0.f}, {}, {}});
	g.path.commands.push_back({Cmd::LineTo, {300.f, 700.f}, {}, {}});
	g.path.commands.push_back({Cmd::LineTo, {550.f, 0.f}, {}, {}});
	g.path.commands.push_back({Cmd::Close, {}, {}, {}});
	const auto glif = rigkit::ufo::writeGlif(g);
	rigkit::ufo::Glif loaded;
	if (!rigkit::ufo::parseGlif(glif, loaded) || loaded.name != "A" ||
		std::fabs(loaded.width - 600.f) > 0.1f || loaded.path.commands.size() < 3) {
		std::fprintf(stderr, "glif roundtrip failed\n");
		return 1;
	}
	std::printf("ufo_smoke ok\n");
	return 0;
}
