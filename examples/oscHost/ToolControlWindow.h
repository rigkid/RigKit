#pragma once

#include "packs/rigImGui/src/IWindow.h"

class OscHost;

class ToolControlWindow : public rigkit::IWindow {
  public:
	explicit ToolControlWindow(OscHost* app);

  protected:
	void renderContents() override;

  private:
	OscHost* m_app = nullptr;
};
