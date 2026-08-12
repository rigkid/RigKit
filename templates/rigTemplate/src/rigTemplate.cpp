#include "rigTemplate.h"
#include <memory>
#include <spdlog/spdlog.h>
#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"

namespace rigkit {

rigTemplate::rigTemplate() : IPack("rigTemplate") {}

bool rigTemplate::init() {
	spdlog::info("[rigTemplate] init");
	return true;
}

void rigTemplate::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	// Data pack: ecs->registerComponent<...>(...);
	// Systems pack: ecs->registerSystem("SMySystem", SystemPhase::Update, SMySystem);
	spdlog::info("[rigTemplate] setup (stub)");
}

} // namespace rigkit

namespace {
struct rigTemplateRegistrar {
	rigTemplateRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigTemplate", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigTemplate>());
		});
	}
};
static rigTemplateRegistrar rigTemplate_auto_reg;
} // namespace
