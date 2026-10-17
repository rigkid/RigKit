#include "rigPsd.h"

#include "core/pack/PackRegistry.h"

#include <spdlog/spdlog.h>

namespace rigkit {

rigPsd::rigPsd() : IPack("rigPsd") {}

bool rigPsd::init() {
	spdlog::info("[rigPsd] init (PSD IO leaf)");
	return true;
}

void rigPsd::setup() {
	spdlog::info("[rigPsd] setup");
}

} // namespace rigkit

namespace {
struct rigPsdRegistrar {
	rigPsdRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigPsd", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigPsd>());
		});
	}
};
static rigPsdRegistrar rigPsd_auto_reg;
} // namespace
