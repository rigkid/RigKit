#include "core/RigKitEngine.h"
#include "app.h"

#include <memory>

int main(int argc, char* argv[]) {
	auto appInstance = std::make_unique<MinimalApp>();
	rigkit::RigKitEngine engine(std::move(appInstance), {}, argc, argv);
	engine.run();
	return 0;
}
