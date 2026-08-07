#pragma once

#include <memory>
#include <string>
#include "core/IApp.h"
#include "core/util/CommandLineArgs.h"

namespace rigkit {
class rigOsc;
}

/// OSC show host: author (rigImGui) vs show (UI-light), multi-instance via network id + UDP.
class OscHost : public rigkit::IApp {
  public:
	OscHost();

	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;

	bool isShowMode() const { return m_showMode; }

	float& masterLevel() { return m_masterLevel; }
	float masterLevel() const { return m_masterLevel; }
	bool& blackout() { return m_blackout; }
	bool blackout() const { return m_blackout; }

	void setShowStatus(const std::string& status) { m_showStatus = status; }
	const std::string& showStatus() const { return m_showStatus; }
	int showHeartbeat() const { return m_showHeartbeat; }

	rigkit::rigOsc* osc() const { return m_osc.get(); }

  protected:
	void setup() override;
	void update(float dt) override;
	void draw() override;
	void exit() override;

  private:
	void bootstrapPacks();
	void setupAuthorUi();
	void syncOscBus();
	bool runOscSmoke();

	bool m_showMode = false;
	bool m_smokeOsc = false;
	float m_time = 0.f;
	float m_hue = 0.35f;

	float m_masterLevel = 1.0f;
	bool m_blackout = false;
	std::string m_showStatus = "idle";
	int m_showHeartbeat = 0;
	float m_heartbeatAccum = 0.f;

	rigkit::CommandLineArgs m_cliArgs;
	std::shared_ptr<rigkit::rigOsc> m_osc;
};
