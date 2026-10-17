#include "rigShowComponent.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "CAudioAnalysis.h"
#include "CPictureAnalysis.h"
#include "CVisualizer.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigShowComponent::rigShowComponent() : IPack("rigShowComponent") {}

bool rigShowComponent::init() {
	spdlog::info("[rigShowComponent] init (data-only)");
	return true;
}

void rigShowComponent::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerComponent<ecs::CVisualizer>("Visualizer", true);
	ecs->registerComponent<ecs::CAudioAnalysis>("AudioAnalysis", true);
	ecs->registerComponent<ecs::CPictureAnalysis>("PictureAnalysis", true);
	spdlog::info("[rigShowComponent] announced show PODs");
}

} // namespace rigkit

namespace {
struct rigShowComponentRegistrar {
	rigShowComponentRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigShowComponent", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigShowComponent>());
		});
	}
};
static rigShowComponentRegistrar rigShowComponent_auto_reg;
} // namespace
