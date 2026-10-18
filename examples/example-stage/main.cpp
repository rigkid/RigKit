#include "core/RigKitEngine.h"
#include "app.h"

#include <memory>
#include <string>

namespace {

bool wantHeadless(int argc, char* argv[]) {
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i] ? argv[i] : "";
		if (arg == "--smoke" || arg.rfind("--smoke=", 0) == 0) {
			return true;
		}
	}
	return false;
}

} // namespace

int main(int argc, char* argv[]) {
	auto app = std::make_unique<StageApp>();
	auto* raw = app.get();
	const bool headless = wantHeadless(argc, argv);
	rigkit::RigKitEngine engine(std::move(app), {}, argc, argv, headless);
	if (!headless) {
		engine.run();
	}
	return raw->smokeFailed() ? 1 : 0;
}
