#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "ecs/MEcs.h"
#include "CDrawStyle.h"
#include "CRigidbody.h"
#include "CSimShape.h"
#include "CSphere.h"
#include "CStagePrefab.h"
#include "CTransform.h"
#include "StageScene.h"

TEST_CASE("feature actors, pose guard, and prefab spawn") {
	rigkit::MEcs ecs;
	stage::registerComponents(ecs);
	stage::addFeatureActors(ecs);

	CHECK(stage::find(ecs, "ball") != entt::null);
	CHECK(stage::find(ecs, "drop") != entt::null);
	CHECK(stage::find(ecs, "stage-cloth") != entt::null);
	CHECK(stage::find(ecs, "missing") == entt::null);
	CHECK(stage::rigidbodyCount(ecs) == 1);
	CHECK(stage::simOwnsPose(ecs, stage::find(ecs, "ball")));
	CHECK(stage::simOwnsPose(ecs, stage::find(ecs, "stage-cloth")));
	CHECK_FALSE(stage::simOwnsPose(ecs, stage::find(ecs, "stage-camera")));
	CHECK_FALSE(stage::simOwnsPose(ecs, stage::find(ecs, "drop")));

	auto camera = stage::find(ecs, "stage-camera");
	CHECK(stage::setPosition(ecs, camera, {1.f, 2.f, 3.f}));
	glm::vec3 cam{0.f};
	CHECK(stage::positionOf(ecs, camera, cam));
	CHECK(cam.x == doctest::Approx(1.f));
	CHECK(cam.z == doctest::Approx(3.f));

	auto ball = stage::find(ecs, "ball");
	glm::vec3 before{0.f};
	CHECK(stage::positionOf(ecs, ball, before));
	CHECK_FALSE(stage::setPosition(ecs, ball, {9.f, 9.f, 9.f}));
	glm::vec3 after{0.f};
	CHECK(stage::positionOf(ecs, ball, after));
	CHECK(after.y == doctest::Approx(before.y));

	glm::vec3 launch{0.f};
	CHECK(stage::velocityOf(ecs, ball, launch));
	CHECK(launch.x == doctest::Approx(0.4f));
	CHECK_FALSE(stage::velocityOf(ecs, camera, launch));

	const auto beforeCount = ecs.getEntityCount();
	auto spawned = stage::spawn(ecs, "drop", {2.f, 5.f, 3.f}, {1.f, 0.f, 0.f});
	CHECK(spawned != entt::null);
	CHECK(ecs.getEntityCount() == beforeCount + 1);
	CHECK(stage::rigidbodyCount(ecs) == 2);
	CHECK(stage::simOwnsPose(ecs, spawned));
	CHECK_FALSE(stage::setPosition(ecs, spawned, {0.f, 0.f, 0.f}));

	glm::vec3 at{0.f};
	CHECK(stage::positionOf(ecs, spawned, at));
	CHECK(at.x == doctest::Approx(2.f));
	CHECK(at.y == doctest::Approx(5.f));
	CHECK(at.z == doctest::Approx(3.f));
	CHECK(ecs.getComponent<rigkit::ecs::CSphere>(spawned).radius == doctest::Approx(0.28f));
	CHECK(ecs.getComponent<rigkit::ecs::CSimShape>(spawned).radius == doctest::Approx(0.28f));
	CHECK(ecs.getComponent<rigkit::ecs::CRigidbody>(spawned).mass == doctest::Approx(0.8f));
	glm::vec3 vel{0.f};
	CHECK(stage::velocityOf(ecs, spawned, vel));
	CHECK(vel.x == doctest::Approx(1.f));
	CHECK(ecs.getComponent<rigkit::ecs::CDrawStyle>(spawned).fillR == doctest::Approx(0.25f));
	CHECK(ecs.getComponent<rigkit::ecs::CDrawStyle>(spawned).fillG == doctest::Approx(0.55f));
	CHECK(ecs.getComponent<rigkit::ecs::CDrawStyle>(spawned).fillB == doctest::Approx(0.95f));
	CHECK(stage::find(ecs, "drop-1") == spawned);
	CHECK(ecs.getComponent<stage::CStagePrefab>(stage::find(ecs, "drop")).name == "drop");
	CHECK(stage::spawn(ecs, "nope", {}, {}) == entt::null);

	auto marker = stage::create(ecs, "marker");
	CHECK(stage::setPosition(ecs, marker, {0.4f, 1.2f, -2.f}));
	glm::vec3 markerAt{0.f};
	CHECK(stage::positionOf(ecs, marker, markerAt));
	CHECK(markerAt.x == doctest::Approx(0.4f));
	stage::destroy(ecs, marker);
	CHECK(stage::find(ecs, "marker") == entt::null);
	stage::destroy(ecs, entt::null);
}
