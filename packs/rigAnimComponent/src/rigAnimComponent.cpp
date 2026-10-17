#include "rigAnimComponent.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "CModBinding.h"
#include "CModLfo.h"
#include "CTween.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigAnimComponent::rigAnimComponent() : IPack("rigAnimComponent") {}

bool rigAnimComponent::init() {
	spdlog::info("[rigAnimComponent] init (data-only)");
	return true;
}

void rigAnimComponent::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerComponent<ecs::CModLfo>("ModLfo", true);
	ecs->registerComponent<ecs::CModBinding>("ModBinding", true);
	ecs->registerComponent<ecs::CTween>("Tween", true);
	spdlog::info("[rigAnimComponent] announced anim PODs");
}

} // namespace rigkit

namespace {
struct rigAnimComponentRegistrar {
	rigAnimComponentRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigAnimComponent", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigAnimComponent>());
		});
	}
};
static rigAnimComponentRegistrar rigAnimComponent_auto_reg;
} // namespace
