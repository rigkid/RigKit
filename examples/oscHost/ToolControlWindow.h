#pragma once

#include "packs/rigImGui/src/ImGuiPanel.h"
class OscHost;

class ToolControlWindow : public rigkit::ImGuiPanel {
  public:
	explicit ToolControlWindow(OscHost* app);

  protected:
	void renderContents() override;

  private:
	OscHost* m_app = nullptr;
};
