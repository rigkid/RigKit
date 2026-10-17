#include "rig/show.h"

#include "ecs/MEcs.h"
#include "CAssetRef.h"
#include "CAudioAnalysis.h"
#include "CPictureAnalysis.h"
#include "rig/create.h"

namespace rig {

entt::entity makeVisualizer(rigkit::MEcs& ecs, const std::string& presetDir,
							const std::string& name) {
	auto e = ecs.createEntity(name.empty() ? "visualizer" : name);
	rigkit::ecs::CVisualizer vis;
	vis.presetDir = presetDir;
	ecs.addComponent<rigkit::ecs::CVisualizer>(e, std::move(vis));
	return e;
}

entt::entity makeVisualizer(const std::string& presetDir, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeVisualizer(*ecs, presetDir, name) : entt::null;
}

entt::entity addVisualizerPreset(rigkit::MEcs& ecs, entt::entity visualizer,
								 const std::string& path, const std::string& name) {
	if (visualizer == entt::null || !ecs.hasComponent<rigkit::ecs::CVisualizer>(visualizer)) {
		return entt::null;
	}
	auto e = ecs.createEntity(name.empty() ? "preset" : name);
	rigkit::ecs::CAssetRef asset;
	asset.kind = rigkit::ecs::CAssetRef::Kind::Preset;
	asset.path = path;
	ecs.addComponent<rigkit::ecs::CAssetRef>(e, std::move(asset));
	ecs.getComponent<rigkit::ecs::CVisualizer>(visualizer).presets.push_back(e);
	return e;
}

entt::entity addVisualizerPreset(entt::entity visualizer, const std::string& path,
								 const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? addVisualizerPreset(*ecs, visualizer, path, name) : entt::null;
}

entt::entity makeAudioAnalysis(rigkit::MEcs& ecs, const std::string& name) {
	auto e = ecs.createEntity(name.empty() ? "audio-analysis" : name);
	rigkit::ecs::CAudioAnalysis a;
	a.features = {"onset", "note", "beat", "color", "temperature"};
	ecs.addComponent<rigkit::ecs::CAudioAnalysis>(e, std::move(a));
	return e;
}

entt::entity makeAudioAnalysis(const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeAudioAnalysis(*ecs, name) : entt::null;
}

entt::entity makePictureAnalysis(rigkit::MEcs& ecs, entt::entity source, const std::string& name) {
	auto e = ecs.createEntity(name.empty() ? "picture-analysis" : name);
	rigkit::ecs::CPictureAnalysis p;
	p.source = source;
	p.features = {"color", "temperature"};
	ecs.addComponent<rigkit::ecs::CPictureAnalysis>(e, std::move(p));
	return e;
}

entt::entity makePictureAnalysis(entt::entity source, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makePictureAnalysis(*ecs, source, name) : entt::null;
}

} // namespace rig
