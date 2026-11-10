#pragma once

#include <cstddef>
#include <entt/entt.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include "core/U_core.h"

class ShaderPreviewWindow;

namespace rigkit {
class Mui;
}

class GlEditorApp : public rigkit::IApp {
  public:
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float dt) override;
	void draw() override {}
	void onUiAttached() override;

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	/** @brief Seed one example file as a `CCode` buffer. Null entity on failure. */
	entt::entity seedShader(const std::string& name, const std::string& relativePath, int order);
	void watchDiskBuffers();
	/** @brief Keep the Code Editor tab and the Shader Preview buffer on the same shader. */
	void syncPreviewWithEditor();
	bool runSmoke();

	rigkit::Mui* m_mui = nullptr;
	ShaderPreviewWindow* m_previewWin = nullptr;
	bool m_smoke = false;
	bool m_smokeOk = false;
	entt::entity m_syncedPreview = entt::null;
	std::string m_syncedEditorBuffer;
	std::unordered_map<std::string, std::filesystem::file_time_type> m_mtimes;
};
