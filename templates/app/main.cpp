#include <memory>
#include "app.h"
#include "core/RigKitEngine.h"

int main(int argc, char* argv[]) {
	auto appInstance = std::make_unique<MyApp>();
	rigkit::RigKitEngine engine(std::move(appInstance), {}, argc, argv);
	engine.run();
	return 0;
}
