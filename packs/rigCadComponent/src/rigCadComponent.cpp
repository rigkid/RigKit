#include "rigCadComponent.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "CCadBoolean.h"
#include "CCadBox.h"
#include "CCadChamfer.h"
#include "CCadCylinder.h"
#include "CCadDimension.h"
#include "CCadExtrude.h"
#include "CCadFillet.h"
#include "CCadReferenceLine.h"
#include "CCadReferencePlane.h"
#include "CCadRevolve.h"
#include "CCadSphere.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigCadComponent::rigCadComponent() : IPack("rigCadComponent") {}

bool rigCadComponent::init() {
	spdlog::info("[rigCadComponent] init (data-only)");
	return true;
}

void rigCadComponent::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerComponent<ecs::CCadBox>("CadBox", true);
	ecs->registerComponent<ecs::CCadCylinder>("CadCylinder", true);
	ecs->registerComponent<ecs::CCadSphere>("CadSphere", true);
	ecs->registerComponent<ecs::CCadExtrude>("CadExtrude", true);
	ecs->registerComponent<ecs::CCadRevolve>("CadRevolve", true);
	ecs->registerComponent<ecs::CCadBoolean>("CadBoolean", true);
	ecs->registerComponent<ecs::CCadFillet>("CadFillet", true);
	ecs->registerComponent<ecs::CCadChamfer>("CadChamfer", true);
	ecs->registerComponent<ecs::CCadDimension>("CadDimension", true);
	ecs->registerComponent<ecs::CCadReferenceLine>("CadReferenceLine", true);
	ecs->registerComponent<ecs::CCadReferencePlane>("CadReferencePlane", true);
	spdlog::info("[rigCadComponent] announced CAD PODs");
}

} // namespace rigkit

namespace {
struct rigCadComponentRegistrar {
	rigCadComponentRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigCadComponent", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigCadComponent>());
		});
	}
};
static rigCadComponentRegistrar rigCadComponent_auto_reg;
} // namespace
