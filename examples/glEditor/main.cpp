#include <memory>
#include "core/RigKitEngine.h"
#include "app.h"

int main(int argc, char* argv[]) {
	auto app = std::make_unique<GlEditorApp>();
	auto* raw = app.get();
	rigkit::RigKitEngine engine(std::move(app), {}, argc, argv);
	engine.run();
	const bool failed = raw->smokeFailed();
	return failed ? 1 : 0;
}
