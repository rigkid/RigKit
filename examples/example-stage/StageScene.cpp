#include "StageScene.h"

#include "CDrawStyle.h"
#include "CLight.h"
#include "CRigidbody.h"
#include "CSimCloth.h"
#include "CSimShape.h"
#include "CSphere.h"
#include "CStagePrefab.h"
#include "CStageScript.h"
#include "CTransform.h"
#include "rig/create.h"
#include "simCreate.h"

namespace stage {

void registerComponents(rigkit::MEcs& ecs) {
	ecs.registerComponent<CStagePrefab>("StagePrefab", false);
	ecs.registerComponent<CStageScript>("StageScript", false);
}

void addFeatureActors(rigkit::MEcs& ecs) {
	rig::sim::makeWorld(ecs, {0.f, -9.81f, 0.f}, 4, "stage-world");
	rig::sim::makePlane(ecs, {0.f, 1.f, 0.f}, 0.f, rig::sim::planeFill(), "stage-ground");
	rig::sim::makeWind(ecs, {1.f, 0.f, 0.2f}, 1.5f, "stage-wind");
	rig::sim::makeCloth(ecs, 8, 5, 0.18f, {-0.7f, 2.4f, -0.4f},
						rigkit::ecs::CSimCloth::Pin::TopCorners, rig::sim::clothFill(),
						"stage-cloth");

	rigkit::ecs::CDrawStyle ball;
	ball.setFillColor(0.95f, 0.45f, 0.18f);
	ball.hasStroke = false;
	rig::sim::makeSphere(ecs, {0.f, 2.6f, 0.6f}, 0.35f, 1.f, {0.4f, 0.f, 0.f}, ball, "ball");

	auto camera = rig::makeCamera(ecs, {3.4f, 2.4f, 4.6f}, true, "stage-camera");
	ecs.getComponent<rigkit::ecs::CTransform>(camera).setEulerRadians({-0.35f, 0.65f, 0.f});

	auto light =
		rig::makeLight(ecs, {0.f, 0.f, 0.f}, rigkit::ecs::CLight::Type::Directional, "stage-light");
	ecs.getComponent<rigkit::ecs::CTransform>(light).setEulerRadians({-0.9f, 0.35f, 0.f});
	auto& lightData = ecs.getComponent<rigkit::ecs::CLight>(light);
	lightData.banded = false;
	lightData.ambient = 0.22f;
	lightData.intensity = 1.15f;

	auto drop = ecs.createEntity("drop");
	CStagePrefab prefab;
	prefab.name = "drop";
	prefab.radius = 0.28f;
	prefab.mass = 0.8f;
	prefab.colorR = 0.25f;
	prefab.colorG = 0.55f;
	prefab.colorB = 0.95f;
	ecs.addComponent(drop, prefab);
	ecs.addComponent(drop, rigkit::ecs::CTransform{});

	auto play = ecs.createEntity("play");
	CStageScript script;
	script.path = "play.py";
	script.enabled = true;
	ecs.addComponent(play, script);
}

entt::entity create(rigkit::MEcs& ecs, const std::string& name) {
	auto entity = ecs.createEntity(name);
	ecs.addComponent(entity, rigkit::ecs::CTransform{});
	return entity;
}

void destroy(rigkit::MEcs& ecs, entt::entity entity) {
	if (!ecs.registry().valid(entity)) {
		return;
	}
	ecs.destroyEntity(entity);
}

entt::entity find(rigkit::MEcs& ecs, const std::string& name) {
	return ecs.findEntity(name);
}

bool simOwnsPose(rigkit::MEcs& ecs, entt::entity entity) {
	if (!ecs.registry().valid(entity)) {
		return false;
	}
	if (ecs.hasComponent<rigkit::ecs::CSimCloth>(entity)) {
		return true;
	}
	return ecs.hasComponent<rigkit::ecs::CRigidbody>(entity) &&
		   ecs.hasComponent<rigkit::ecs::CSimShape>(entity);
}

bool positionOf(rigkit::MEcs& ecs, entt::entity entity, glm::vec3& out) {
	if (!ecs.registry().valid(entity) || !ecs.hasComponent<rigkit::ecs::CTransform>(entity)) {
		return false;
	}
	out = ecs.getComponent<rigkit::ecs::CTransform>(entity).position;
	return true;
}

bool setPosition(rigkit::MEcs& ecs, entt::entity entity, glm::vec3 position) {
	if (simOwnsPose(ecs, entity)) {
		return false;
	}
	if (!ecs.registry().valid(entity) || !ecs.hasComponent<rigkit::ecs::CTransform>(entity)) {
		return false;
	}
	ecs.getComponent<rigkit::ecs::CTransform>(entity).position = position;
	return true;
}

bool velocityOf(rigkit::MEcs& ecs, entt::entity entity, glm::vec3& out) {
	if (!ecs.registry().valid(entity) || !ecs.hasComponent<rigkit::ecs::CRigidbody>(entity)) {
		return false;
	}
	out = ecs.getComponent<rigkit::ecs::CRigidbody>(entity).velocity;
	return true;
}

entt::entity spawn(rigkit::MEcs& ecs, const std::string& prefab, glm::vec3 at, glm::vec3 velocity) {
	entt::entity recipe = entt::null;
	for (auto entity : ecs.view<CStagePrefab>()) {
		if (ecs.getComponent<CStagePrefab>(entity).name == prefab) {
			recipe = entity;
			break;
		}
	}
	if (recipe == entt::null) {
		return entt::null;
	}

	const CStagePrefab& spec = ecs.getComponent<CStagePrefab>(recipe);
	rigkit::ecs::CDrawStyle style;
	style.setFillColor(spec.colorR, spec.colorG, spec.colorB);
	style.hasStroke = false;

	int n = 1;
	std::string name;
	do {
		name = spec.name + "-" + std::to_string(n++);
	} while (ecs.findEntity(name) != entt::null);

	return rig::sim::makeSphere(ecs, at, spec.radius, spec.mass, velocity, style, name);
}

int rigidbodyCount(rigkit::MEcs& ecs) {
	int count = 0;
	for (auto entity : ecs.view<rigkit::ecs::CRigidbody>()) {
		(void)entity;
		++count;
	}
	return count;
}

} // namespace stage
