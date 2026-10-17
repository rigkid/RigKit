#pragma once

#include "core/U_core.h"
#include "core/util/View2D.h"

#include <string>

class SchDoc;

class KicadSchApp : public rigkit::IApp {
  public:
	KicadSchApp();
	~KicadSchApp() override;

	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float) override;
	void draw() override {}

	void addWheel(float y) { m_wheel += y; }
	void addChar(char c) { m_typed.push_back(c); }
	bool smokeFailed() const { return m_smoke && !m_smokeOk; }

  private:
	void present();
	bool pointer(float& sx, float& sy, float& kx, float& ky);
	void onKey(int key);

	SchDoc* m_doc = nullptr;
	rigkit::View2D m_view;
	std::string m_path;
	bool m_smoke = false;
	bool m_smokeOk = false;
	bool m_fitted = false;
	float m_wheel = 0.f;
	float m_lastSx = 0.f;
	float m_lastSy = 0.f;
	bool m_panning = false;
	bool m_leftWas = false;
	bool m_bracketLeft = false;
	bool m_bracketRight = false;
	bool m_keyF = false;
	bool m_keyS = false;
	bool m_keyA = false;
	bool m_keyH = false;
	bool m_keyEsc = false;
	bool m_keyEnter = false;
	bool m_keyBack = false;
	bool m_keyUp = false;
	bool m_keyDown = false;
	bool m_ignoreA = false;
	std::string m_typed;
	int m_gridIndex = 1;
};
