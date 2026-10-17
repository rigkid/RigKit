#include "rigMusicComponent.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "CMusicClock.h"
#include "CMusicTransport.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigMusicComponent::rigMusicComponent() : IPack("rigMusicComponent") {}

bool rigMusicComponent::init() {
	spdlog::info("[rigMusicComponent] init (data-only)");
	return true;
}

void rigMusicComponent::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerComponent<ecs::CMusicClock>("MusicClock", true);
	ecs->registerComponent<ecs::CMusicTransport>("MusicTransport", true);
	spdlog::info("[rigMusicComponent] announced music PODs");
}

} // namespace rigkit

namespace {
struct rigMusicComponentRegistrar {
	rigMusicComponentRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigMusicComponent", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigMusicComponent>());
		});
	}
};
static rigMusicComponentRegistrar rigMusicComponent_auto_reg;
} // namespace
