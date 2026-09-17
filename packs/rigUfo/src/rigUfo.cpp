#include "rigUfo.h"

#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigUfo::rigUfo() : IPack("rigUfo") {}

bool rigUfo::init() {
	spdlog::info("[rigUfo] init");
	return true;
}

void rigUfo::setup() {
	spdlog::info("[rigUfo] setup (IO)");
}

} // namespace rigkit

namespace {
struct rigUfoRegistrar {
	rigUfoRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigUfo", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigUfo>());
		});
	}
};
static rigUfoRegistrar rigUfo_auto_reg;
} // namespace
