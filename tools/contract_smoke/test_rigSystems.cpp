#include <doctest.h>

#include "fixture.h"

using namespace rigkit;
using namespace rigkit::smoke;

TEST_CASE("rigSystems registers Update/Draw systems without replacing data catalog") {
	SpineFixture f;
	auto& ecs = f.ecs();

	// Data catalog still owned by rigComponent registrations.
	REQUIRE(f.findType("Transform") != nullptr);
	REQUIRE(f.findType("Rectangle") != nullptr);
	CHECK(f.findType("Transform")->portable);
	CHECK(f.findType("Rectangle")->portable);

	REQUIRE(ecs.hasSystem("SHierarchy"));
	REQUIRE(ecs.hasSystem("SCanvasUpdate"));
	REQUIRE(ecs.hasSystem("SCanvasRender"));
	REQUIRE(ecs.hasSystem("SShapeRendering"));

	// Systems pack must not register component type names as systems.
	CHECK_FALSE(ecs.hasSystem("Transform"));
	CHECK_FALSE(ecs.hasSystem("Rectangle"));

	bool sawUpdate = false;
	bool sawDraw = false;
	for (const auto& entry : ecs.systems()) {
		if (entry.phase == SystemPhase::Update) {
			sawUpdate = true;
		}
		if (entry.phase == SystemPhase::Draw) {
			sawDraw = true;
		}
	}
	CHECK(sawUpdate);
	CHECK(sawDraw);
}
