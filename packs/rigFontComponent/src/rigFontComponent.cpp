#include "rigFontComponent.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/pack/PackRegistry.h"
#include "ecs/MEcs.h"
#include "CFontAnchor.h"
#include "CFontAvar.h"
#include "CFontAxis.h"
#include "CFontCell.h"
#include "CFontComponent.h"
#include "CFontFace.h"
#include "CFontGlyph.h"
#include "CFontGroup.h"
#include "CFontKern.h"
#include "CFontLayer.h"
#include "CFontMaster.h"
#include "FontSerializers.h"
#include "rigProject.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigFontComponent::rigFontComponent() : IPack("rigFontComponent") {}

bool rigFontComponent::init() {
	spdlog::info("[rigFontComponent] init (data-only)");
	return true;
}

void rigFontComponent::setup() {
	auto* engine = getEngine();
	if (!engine) {
		return;
	}
	auto* ecs = engine->getECSManager();
	if (!ecs) {
		return;
	}

	ecs->registerComponent<ecs::CFontFace>("FontFace", true);
	ecs->registerComponent<ecs::CFontLayer>("FontLayer", true);
	ecs->registerComponent<ecs::CFontGlyph>("FontGlyph", true);
	ecs->registerComponent<ecs::CFontComponent>("FontComponent", true);
	ecs->registerComponent<ecs::CFontAnchor>("FontAnchor", true);
	ecs->registerComponent<ecs::CFontKern>("FontKern", true);
	ecs->registerComponent<ecs::CFontGroup>("FontGroup", true);
	ecs->registerComponent<ecs::CFontAxis>("FontAxis", true);
	ecs->registerComponent<ecs::CFontAvar>("FontAvar", true);
	ecs->registerComponent<ecs::CFontCell>("FontCell", true);
	ecs->registerComponent<ecs::CFontMaster>("FontMaster", true);

	if (auto* packs = engine->getPackManager()) {
		if (auto doc = std::dynamic_pointer_cast<rigProject>(packs->getPack("rigProject"))) {
			font::registerProjectSerializers(doc->serializer().registry());
			spdlog::info("[rigFontComponent] registered font codecs on rigProject");
		} else {
			spdlog::warn("[rigFontComponent] rigProject not loaded - font .rig codecs skipped");
		}
	}
}

} // namespace rigkit

namespace {
struct rigFontComponentRegistrar {
	rigFontComponentRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigFontComponent", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigFontComponent>());
		});
	}
};
static rigFontComponentRegistrar rigFontComponent_auto_reg;
} // namespace
