#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>

#include "core/ISettings.h"
#include "rendering/Graphics.h"
#include "rendering/IRenderer.h"

/**
 * The Rendering Manager is the sole owner/manager of all engine-level IRenderer
 * instances. All renderer creation, switching, and access should go through here.
 */
namespace rigkit {
class MRendering : public ISettings {
  public:
	MRendering();
	~MRendering();
	void init() {}
	void shutdown() {}

	// Main renderer management
	void setMainRenderer(std::shared_ptr<IRenderer> renderer);
	std::shared_ptr<IRenderer> getMainRenderer() const;

	// Renderer management
	std::shared_ptr<IRenderer> getRenderer(entt::entity entity);
	std::shared_ptr<IRenderer> createRenderer(entt::entity entity, RendererType type, int width,
											  int height);
	void destroyRenderer(entt::entity entity);

	// Graphics management
	std::shared_ptr<Graphics> getGraphics(entt::entity entity);
	void addGraphics(entt::entity entity, std::shared_ptr<Graphics> graphics);
	void removeGraphics(entt::entity entity);

	// Resource cleanup
	void cleanup();

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	std::shared_ptr<IRenderer> m_mainRenderer;
	std::unordered_map<entt::entity, std::shared_ptr<IRenderer>> m_renderers;
	std::unordered_map<entt::entity, std::shared_ptr<Graphics>> m_graphics;
};

std::shared_ptr<IRenderer> createRenderer(RendererType type);
RendererType getRendererTypeFromString(const std::string& name);
std::vector<std::string> getAvailableRendererNames();

} // namespace rigkit
