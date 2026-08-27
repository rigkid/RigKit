#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <string>

#include "IWindow.h"
#include "ShaderPreview.h"

/**
 * @brief Live shader preview panel - compile / auto / pause / FPS / errors.
 */
class ShaderPreviewWindow : public rigkit::IWindow {
  public:
	ShaderPreviewWindow();

	ShaderPreview& preview() { return m_preview; }
	const ShaderPreview& preview() const { return m_preview; }

	void setPreviewEntity(entt::entity e) { m_entity = e; }
	entt::entity previewEntity() const { return m_entity; }

	bool autoCompile() const { return m_autoCompile; }
	void setAutoCompile(bool v) { m_autoCompile = v; }

	/** @brief Debounced auto-compile from CCode + render FBO for this frame. */
	void tick(float dt);

	/** @brief Compile now from the selected CCode (or forceSource if non-empty). */
	bool compileNow(const std::string& forceSource = {});

  protected:
	void renderContents() override;

  private:
	ShaderPreview m_preview;
	entt::entity m_entity = entt::null;
	bool m_autoCompile = true;
	float m_debounce = 0.f;
	uint32_t m_seenEpoch = 0;
	bool m_seenDirty = false;
	std::string m_seenTextHash;
	glm::vec4 m_mouse{0.f};
	bool m_mouseDown = false;
};
