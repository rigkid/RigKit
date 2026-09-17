#include "rigFont.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigFont::rigFont() : IPack("rigFont") {}

bool rigFont::init() {
	spdlog::info("[rigFont] init");
	return true;
}

void rigFont::setup() {
	spdlog::info("[rigFont] setup (shape / reconstruct)");
}

} // namespace rigkit

namespace {
struct rigFontRegistrar {
	rigFontRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigFont", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigFont>());
		});
	}
};
static rigFontRegistrar rigFont_auto_reg;
} // namespace
