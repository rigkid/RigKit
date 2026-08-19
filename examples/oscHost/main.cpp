#include "core/RigKitEngine.h"
#include "app.h"

#include <memory>

int main(int argc, char* argv[]) {
	auto app = std::make_unique<OscHost>();
	rigkit::RigKitEngine engine(std::move(app), {}, argc, argv);
	engine.run();
	return 0;
}
