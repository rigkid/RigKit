#include "core/RigKitEngine.h"
#include "app.h"

#include <memory>

int main(int argc, char* argv[]) {
	auto app = std::make_unique<PixelEditorApp>();
	auto* raw = app.get();
	rigkit::RigKitEngine engine(std::move(app), {}, argc, argv);
	engine.run();
	return raw->smokeFailed() ? 1 : 0;
}
