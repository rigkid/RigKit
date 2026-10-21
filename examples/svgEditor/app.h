#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/U_core.h"

namespace rigkit {
class rigPlotter;
class rigProject;
class rigTraceUi;
} // namespace rigkit

class SvgEditorApp : public rigkit::IApp {
  public:
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override;
	void draw() override {}

	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void seedDemoLayers();
	void syncTraceSource();
	void clearTraceSource();
	bool runSmoke();
	std::string documentPathHint() const;

	std::shared_ptr<rigkit::rigPlotter> m_plotter;
	std::shared_ptr<rigkit::rigProject> m_document;
	std::shared_ptr<rigkit::rigTraceUi> m_traceUi;
	std::string m_traceHref;
	int m_traceLayer = -1;
	int m_traceImage = -1;
	float m_traceX = 0.f;
	float m_traceY = 0.f;
	float m_traceW = 0.f;
	float m_traceH = 0.f;
	std::vector<unsigned int> m_traceApplied;
	bool m_smoke = false;
	bool m_smokeOk = false;
};
