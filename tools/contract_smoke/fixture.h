#pragma once

#include <doctest.h>

#include <memory>
#include <string>
#include <vector>

#include "SmokeApp.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "ecs/MEcs.h"
#include "ecs/PropertyReflection.h"
#include "rigComponent.h"
#include "rigProject.h"
#include "rigSystems.h"

namespace rigkit {
namespace smoke {

inline bool propPointsIntoComponent(const sProp& prop, const void* component, std::size_t size) {
	if (!prop.data || !component || size == 0) {
		return false;
	}
	const auto* base = static_cast<const unsigned char*>(component);
	const auto* ptr = static_cast<const unsigned char*>(prop.data);
	return ptr >= base && ptr < base + size;
}

/** Headless engine + spine packs (rigComponent → rigSystems → rigProject). */
struct SpineFixture {
	std::unique_ptr<RigKitEngine> engine;

	SpineFixture() {
		engine = std::make_unique<RigKitEngine>(std::make_unique<SmokeApp>(), json{}, 0, nullptr,
												true);
		auto* packs = engine->getPackManager();
		REQUIRE(packs != nullptr);

		// Construct packs directly — STATIC pack libs may drop PackRegistry
		// self-registration object files when nothing references them.
		packs->registerPack<rigComponent>();
		packs->registerPack<rigSystems>();
		packs->registerPack<rigProject>();
		REQUIRE(packs->initAll());
		packs->setupAll();
	}

	MEcs& ecs() {
		auto* e = engine->getECSManager();
		REQUIRE(e != nullptr);
		return *e;
	}

	const ComponentTypeInfo* findType(const std::string& name) {
		for (const auto& info : ecs().componentTypes()) {
			if (info.name == name) {
				return &info;
			}
		}
		return nullptr;
	}
};

} // namespace smoke
} // namespace rigkit
