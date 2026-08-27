#include "rendering/MRendering.h"

#include "core/canvas/Canvas.h"
#include "core/ISettings.h"
#include "rendering/RendererRegistry.h"

namespace rigkit {

MRendering::MRendering() {}

MRendering::~MRendering() {
	cleanup();
}

std::shared_ptr<IRenderer> MRendering::getRenderer(entt::entity entity) {
	auto it = m_renderers.find(entity);
	if (it != m_renderers.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<IRenderer> MRendering::createRenderer(entt::entity entity, RendererType type,
													  int width, int height) {
	auto uniqueRenderer = RendererRegistry::instance().create(type);
	if (uniqueRenderer && uniqueRenderer->initialize(width, height)) {
		std::shared_ptr<IRenderer> renderer = std::move(uniqueRenderer);
		m_renderers[entity] = renderer;
		return renderer;
	}
	return nullptr;
}

void MRendering::destroyRenderer(entt::entity entity) {
	auto it = m_renderers.find(entity);
	if (it != m_renderers.end()) {
		m_renderers.erase(it);
	}
}

void MRendering::cleanup() {
	m_renderers.clear();
	m_graphics.clear();
}

// Graphics management
std::shared_ptr<Graphics> MRendering::getGraphics(entt::entity entity) {
	auto it = m_graphics.find(entity);
	if (it != m_graphics.end()) {
		return it->second;
	}
	return nullptr;
}

void MRendering::addGraphics(entt::entity entity, std::shared_ptr<Graphics> graphics) {
	m_graphics[entity] = graphics;
}

void MRendering::removeGraphics(entt::entity entity) {
	auto it = m_graphics.find(entity);
	if (it != m_graphics.end()) {
		m_graphics.erase(it);
	}
}

void MRendering::setMainRenderer(std::shared_ptr<IRenderer> renderer) {
	m_mainRenderer = renderer;
}

std::shared_ptr<IRenderer> MRendering::getMainRenderer() const {
	return m_mainRenderer;
}

rigkit::json MRendering::getSettings() const {
	// Association metadata only - not the main window renderer, Graphics
	// wrappers, or ECS scene (those are engine / runtime / rigProject).
	rigkit::json j;
	j["renderers"] = rigkit::json::array();
	for (const auto& [entity, renderer] : m_renderers) {
		if (renderer) {
			rigkit::json rj;
			rj["entity"] = static_cast<uint32_t>(entity);
			rj["type"] = static_cast<int>(renderer->getType());
			j["renderers"].push_back(rj);
		}
	}
	return j;
}

void MRendering::setSettings(const rigkit::json& settings) {
	m_renderers.clear();
	if (settings.contains("renderers")) {
		for (const auto& rj : settings["renderers"]) {
			entt::entity entity = static_cast<entt::entity>(rj["entity"].get<uint32_t>());
			RendererType type = static_cast<RendererType>(rj["type"].get<int>());
			createRenderer(entity, type, CanvasSettings().width, CanvasSettings().height);
		}
	}
}

} // namespace rigkit

// -----------------------------------------------------------------------------
// Helper functions - global to match IRenderer.h declarations.

#include <string>
#include <vector>

std::shared_ptr<IRenderer> createRenderer(RendererType type) {
	return RendererRegistry::instance().create(type);
}

RendererType getRendererTypeFromString(const std::string& name) {
	if (name == "opengl" || name == "OpenGL") {
		return RendererType::OpenGL;
	} else if (name == "blend2d" || name == "Blend2D") {
		return RendererType::Blend2D;
	}
	return RendererType::OpenGL; // Default
}

std::vector<std::string> getAvailableRendererNames() {
	std::vector<std::string> names;
	for (RendererType type : RendererRegistry::instance().registeredTypes()) {
		switch (type) {
		case RendererType::OpenGL:
			names.emplace_back("OpenGL");
			break;
		case RendererType::Blend2D:
			names.emplace_back("Blend2D");
			break;
		}
	}
	return names;
}
