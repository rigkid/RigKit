#include "app.h"
#include "core/RigKitEngine.h"

#include <memory>

int main(int argc, char* argv[]) {
	auto app = std::make_unique<ShapeApp>();
	rigkit::RigKitEngine engine(std::move(app), {}, argc, argv);
	engine.run();
	return 0;
}
