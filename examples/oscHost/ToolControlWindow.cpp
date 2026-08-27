#include "ToolControlWindow.h"

#include "packs/rigOsc/src/CNetworkIdentity.h"
#include "packs/rigOsc/src/COscEndpoint.h"
#include "packs/rigOsc/src/rigOsc.h"
#include "app.h"

#include <cstring>
#include <imgui.h>

namespace {

bool inputTextStd(const char* label, std::string& value) {
	char buf[256];
	std::strncpy(buf, value.c_str(), sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	if (ImGui::InputText(label, buf, sizeof(buf))) {
		value = buf;
		return true;
	}
	return false;
}

} // namespace

ToolControlWindow::ToolControlWindow(OscHost* app)
	: rigkit::IWindow("Show Control", 0), m_app(app) {}

void ToolControlWindow::renderContents() {
	ImGui::TextUnformatted("oscHost - show / network controls");
	ImGui::Separator();

	if (!m_app) {
		return;
	}

	ImGui::SliderFloat("Master", &m_app->masterLevel(), 0.f, 1.f);
	ImGui::Checkbox("Blackout", &m_app->blackout());
	ImGui::ColorEdit3("Color", m_app->color(), ImGuiColorEditFlags_Float);

	ImGui::Separator();
	ImGui::Text("Show status: %s", m_app->showStatus().c_str());
	ImGui::Text("Heartbeat: %d", m_app->showHeartbeat());
	if (ImGui::Button("Status: running")) {
		m_app->setShowStatus("running");
	}
	ImGui::SameLine();
	if (ImGui::Button("Status: idle")) {
		m_app->setShowStatus("idle");
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Network / OSC (rigOsc)");
	auto* osc = m_app->osc();
	if (!osc) {
		ImGui::TextDisabled("rigOsc not loaded");
		return;
	}

	auto& id = osc->identity();
	auto& ep = osc->endpoint();

	inputTextStd("Network ID", id.networkId);
	inputTextStd("Bind Address", id.bindAddress);
	ImGui::Checkbox("Listen", &ep.listenEnabled);
	ImGui::SameLine();
	ImGui::Checkbox("Send", &ep.sendEnabled);
	ImGui::InputInt("Listen Port", &ep.listenPort);
	inputTextStd("Send Host", ep.sendHost);
	ImGui::InputInt("Send Port", &ep.sendPort);
	inputTextStd("Address Prefix", ep.addressPrefix);

	if (ImGui::Button("Apply Endpoint")) {
		if (!osc->applyEndpoint()) {
			ImGui::OpenPopup("osc_bind_err");
		}
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(osc->listening() ? "Listening" : "Not listening");
	if (!osc->lastError().empty()) {
		ImGui::TextColored(ImVec4(1.f, 0.4f, 0.3f, 1.f), "%s", osc->lastError().c_str());
	}

	if (ImGui::BeginPopup("osc_bind_err")) {
		ImGui::Text("Bind failed: %s", osc->lastError().c_str());
		ImGui::EndPopup();
	}

	ImGui::TextWrapped("Directed: %s/<id>/master|blackout|color|...  "
					   "Broadcast: %s/... (no id). Bus sends append sender id. "
					   "Peer follows on the heartbeat tick (~1s).",
					   ep.addressPrefix.c_str(), ep.addressPrefix.c_str());
}
