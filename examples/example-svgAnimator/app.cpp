#include "app.h"

#include "rendering/U_gladGlfw.h"
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "core/util/CommandLineArgs.h"
#include "packs/rigComponent/src/CDrawStyle.h"
#include "packs/rigComponent/src/CPath.h"
#include "packs/rigComponent/src/CTransform.h"
#include "packs/rigComponent/src/CTween.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigSvg/src/SvgLoader.h"
#include "packs/rigSvg/src/rigSvg.h"
#include "packs/rigSystems/src/STweens.h"
#include "packs/rigSystems/src/rigSystems.h"

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace {

rigkit::ecs::CDrawStyle styleFor(const rigkit::plot::PathSourceLayer& layer,
								 const rigkit::ecs::CPaths& bag, size_t i) {
	rigkit::ecs::CDrawStyle style;
	style.hasFill = i >= bag.pathFilled.size() || bag.pathFilled[i] != 0;
	style.hasStroke = i < bag.pathStroked.size() && bag.pathStroked[i] != 0;
	glm::vec4 fill = layer.layerColor;
	if (i < bag.pathColors.size()) {
		fill = bag.pathColors[i];
	}
	style.setFillColor(fill.r, fill.g, fill.b, fill.a);
	if (i < bag.pathStrokeColors.size()) {
		const glm::vec4& stroke = bag.pathStrokeColors[i];
		style.setStrokeColor(stroke.r, stroke.g, stroke.b, stroke.a);
	}
	if (i < bag.pathStrokeWidths.size()) {
		style.setStrokeWidth(bag.pathStrokeWidths[i]);
	}
	return style;
}

} // namespace

SvgAnimatorApp::SvgAnimatorApp() {
	window().width = 800;
	window().height = 600;
	window().title = "example-svgAnimator";
}

void SvgAnimatorApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	IApp::parseCommandLineArgs(args);
	if (args.hasFlag("smoke")) {
		m_smoke = true;
	}
}

bool SvgAnimatorApp::loadDemo() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return false;
	}

	const std::string path = AppPaths::joinPath(AppPaths::getDataDir(), "demo.svg");
	rigkit::plot::ParsedPathDoc doc;
	if (!rigkit::svg::loadFile(path, doc) || doc.empty()) {
		spdlog::error("example-svgAnimator: failed to load {}", path);
		return false;
	}

	int count = 0;
	entt::entity last = entt::null;
	for (const auto& layer : doc.layers) {
		const auto& bag = layer.paths;
		for (size_t i = 0; i < bag.paths.size(); ++i) {
			if (bag.paths[i].empty()) {
				continue;
			}
			auto e = ecs->createEntity("path-" + std::to_string(count));
			ecs->addComponent<rigkit::ecs::CTransform>(e, {});
			ecs->addComponent<rigkit::ecs::CPath>(e, bag.paths[i]);
			ecs->addComponent<rigkit::ecs::CDrawStyle>(e, styleFor(layer, bag, i));
			last = e;
			++count;
		}
	}
	if (count == 0 || last == entt::null) {
		spdlog::error("example-svgAnimator: SVG had no paths");
		return false;
	}

	ecs->setEntityName(last, "bob");

	rigkit::ecs::CTween tween;
	tween.target = "bob";
	tween.propertyKey = "position.x";
	tween.from = 0.f;
	tween.to = 80.f;
	tween.duration = 2.f;
	tween.easing = "ease-in-out";
	tween.loop = true;
	auto slide = ecs->createEntity("bob-slide");
	ecs->addComponent<rigkit::ecs::CTween>(slide, tween);
	spdlog::info("example-svgAnimator: {} paths, bob slides on position.x", count);
	return true;
}

bool SvgAnimatorApp::runSmoke() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		spdlog::error("example-svgAnimator smoke: no ECS");
		return false;
	}
	const auto bob = ecs->findEntity("bob");
	if (bob == entt::null || !ecs->hasComponent<rigkit::ecs::CTransform>(bob)) {
		spdlog::error("example-svgAnimator smoke: bob missing");
		return false;
	}
	const float before = ecs->getComponent<rigkit::ecs::CTransform>(bob).position.x;
	rigkit::ecs::STweens(*ecs, 0.5f);
	const float after = ecs->getComponent<rigkit::ecs::CTransform>(bob).position.x;
	if (!(after > before + 1.f)) {
		spdlog::error("example-svgAnimator smoke: position.x {:.3f} to {:.3f}", before, after);
		return false;
	}
	spdlog::info("example-svgAnimator smoke: position.x {:.3f} to {:.3f}", before, after);
	return true;
}

void SvgAnimatorApp::setup() {
	spdlog::info("example-svgAnimator - SVG paths driven by CTween");
	m_engine->setClearColor(0.09f, 0.09f, 0.11f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack<rigkit::rigComponent>();
	packs->registerPack<rigkit::rigSystems>();
	packs->registerPack<rigkit::rigSvg>();
	packs->initAll();
	packs->setupAll();

	const bool loaded = loadDemo();
	if (!m_smoke) {
		if (!loaded) {
			spdlog::error("example-svgAnimator: demo SVG did not load");
		}
		return;
	}

	m_smokeOk = loaded && runSmoke();
	if (m_smokeOk) {
		spdlog::info("example-svgAnimator --smoke ok");
	} else {
		spdlog::error("example-svgAnimator --smoke failed");
	}
	if (auto* win = m_engine->getWindow()) {
		glfwSetWindowShouldClose(win, GLFW_TRUE);
	}
}
