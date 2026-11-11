#pragma once

#include <memory>
#include <string>

#include "core/U_core.h"
#include "packs/rigProject/src/Rigz.h"

namespace rigkit {
class Mui;
class rigPython;
class rigProject;
} // namespace rigkit

class StageApp : public rigkit::IApp {
  public:
	~StageApp() override;
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float dt) override;
	void draw() override {}
	void onUiAttached() override;

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }
	bool presentFailed() const { return m_present && !m_presentOk; }

  private:
	bool runSmoke();
	bool runOpenSmoke();
	bool openDocument(const std::string& path);

	rigkit::Mui* m_mui = nullptr;
	bool m_smoke = false;
	bool m_smokeOk = false;
	bool m_play = false;
	bool m_present = false;
	bool m_presentOk = false;
	int m_presentFrames = 0;
	std::string m_openPath;
	std::string m_scriptError;
	std::shared_ptr<rigkit::rigPython> m_python;
	std::shared_ptr<rigkit::rigProject> m_document;
	rigkit::project::PackageOpen m_package;
};
