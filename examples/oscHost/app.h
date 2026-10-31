#pragma once

#include <memory>
#include <string>

#include <entt/entt.hpp>

#include "core/IApp.h"
#include "core/util/CommandLineArgs.h"

namespace rigkit {
class rigOsc;
namespace ecs {
class SEvent;
struct CDrawStyle;
struct CRenderBlend;
} // namespace ecs
} // namespace rigkit

/// OSC show host: author (rigImGui) vs show (UI-light), multi-instance via network id + UDP.
class OscHost : public rigkit::IApp {
  public:
	OscHost();

	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;

	bool isShowMode() const { return m_showMode; }

	float& masterLevel();
	float masterLevel() const;
	bool& blackout() { return m_blackout; }
	bool blackout() const { return m_blackout; }
	float* color();

	void setShowStatus(const std::string& status) { m_showStatus = status; }
	const std::string& showStatus() const { return m_showStatus; }
	int showHeartbeat() const { return m_showHeartbeat; }

	rigkit::rigOsc* osc() const { return m_osc.get(); }

  protected:
	void setup() override;
	void update(float dt) override;
	void draw() override;
	void exit() override;
	void registerUIActions(rigkit::ecs::SEvent& events) override;

  private:
	void bootstrapPacks();
	void setupLook();
	void setupLookPanel();
	void setupAuthorUi();
	void registerShowBus();
	void publishShowBus();
	bool runOscSmoke();

	rigkit::ecs::CDrawStyle& lookStyle() const;
	rigkit::ecs::CRenderBlend& lookBlend() const;

	bool m_showMode = false;
	bool m_smokeOsc = false;

	entt::entity m_look = entt::null;
	bool m_blackout = false;
	std::string m_showStatus = "idle";
	int m_showHeartbeat = 0;
	float m_heartbeatAccum = 0.f;

	rigkit::CommandLineArgs m_cliArgs;
	std::shared_ptr<rigkit::rigOsc> m_osc;
};

