#include "StageProp.h"

#include "AssimpIO.h"
#include "CSelectable.h"
#include "CSelection.h"
#include "CTransform.h"

namespace stage {

bool addProp(rigkit::MEcs& ecs, const std::string& path, std::string* err) {
	rig::assimp::IoResult loaded;
	rigkit::ecs::CDrawStyle style;
	style.setFillColor(0.15f, 0.75f, 0.35f);
	style.hasStroke = false;
	auto prop = rig::assimp::makeMeshFromFile(ecs, path, style, "prop", &loaded);
	if (prop == entt::null) {
		if (err) {
			*err = loaded.error.empty() ? "prop mesh failed to load" : loaded.error;
		}
		return false;
	}
	auto& xf = ecs.getComponent<rigkit::ecs::CTransform>(prop);
	xf.position = {-1.4f, 0.f, 1.1f};

	ecs.addComponent(prop, rigkit::ecs::CSelectable{});
	rigkit::ecs::CSelection selection;
	selection.isSelected = true;
	ecs.addComponent(prop, selection);
	return true;
}

} // namespace stage
