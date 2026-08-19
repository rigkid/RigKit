#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include "core/U_core.h"

class ShaderPreviewWindow;

class GlEditorApp : public rigkit::IApp {
  public:
	GlEditorApp();
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float dt) override;
	void draw() override {}

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	bool seedShader(const std::string& name, const std::string& relativePath, int order);
	void watchDiskBuffers();
	bool runSmoke();

	ShaderPreviewWindow* m_previewWin = nullptr;
	bool m_smoke = false;
	bool m_smokeOk = false;
	std::unordered_map<std::string, std::filesystem::file_time_type> m_mtimes;
};
