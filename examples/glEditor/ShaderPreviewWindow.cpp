#include "ShaderPreviewWindow.h"

#include "core/RigKitEngine.h"
#include "ecs/MEcs.h"
#include "packs/rigComponent/src/CCode.h"

#include <imgui.h>
#include <algorithm>
#include <functional>
#include <vector>

namespace {

std::string shortHash(const std::string& s) {
	return std::to_string(std::hash<std::string>{}(s)) + ":" + std::to_string(s.size());
}

} // namespace

ShaderPreviewWindow::ShaderPreviewWindow() : IWindow("Shader Preview") {
	setCategory("glEditor");
}

bool ShaderPreviewWindow::compileNow(const std::string& forceSource) {
	auto* engine = getEngine();
	rigkit::MEcs* ecs = engine ? engine->getECSManager() : nullptr;
	if (!ecs) {
		return false;
	}

	std::string src = forceSource;
	if (src.empty()) {
		if (m_entity == entt::null || !ecs->hasComponent<rigkit::ecs::CCode>(m_entity)) {
			return false;
		}
		src = ecs->getComponent<rigkit::ecs::CCode>(m_entity).text;
	}
	if (src.empty()) {
		return false;
	}
	const bool ok = m_preview.compile(src);
	if (m_entity != entt::null && ecs->hasComponent<rigkit::ecs::CCode>(m_entity)) {
		auto& code = ecs->getComponent<rigkit::ecs::CCode>(m_entity);
		m_seenEpoch = code.epoch;
		m_seenDirty = code.dirty;
		m_seenTextHash = shortHash(code.text);
		if (ok) {
			code.dirty = false;
		}
	}
	m_debounce = 0.f;
	return ok;
}

void ShaderPreviewWindow::tick(float dt) {
	m_preview.advanceTime(dt);

	auto* engine = getEngine();
	rigkit::MEcs* ecs = engine ? engine->getECSManager() : nullptr;
	if (!ecs) {
		return;
	}

	if (m_autoCompile && m_entity != entt::null &&
		ecs->hasComponent<rigkit::ecs::CCode>(m_entity)) {
		const auto& code = ecs->getComponent<rigkit::ecs::CCode>(m_entity);
		const std::string hash = shortHash(code.text);
		const bool changed =
			code.epoch != m_seenEpoch || code.dirty != m_seenDirty || hash != m_seenTextHash;
		if (changed) {
			m_debounce += dt;
			if (m_debounce >= 0.3f) {
				compileNow();
			}
		} else {
			m_debounce = 0.f;
		}
	}
}

void ShaderPreviewWindow::renderContents() {
	auto* engine = getEngine();
	rigkit::MEcs* ecs = engine ? engine->getECSManager() : nullptr;

	if (ecs) {
		std::vector<entt::entity> buffers;
		for (auto e : ecs->registry().view<rigkit::ecs::CCode>()) {
			const auto& c = ecs->getComponent<rigkit::ecs::CCode>(e);
			if (c.language == "glsl") {
				buffers.push_back(e);
			}
		}
		std::sort(buffers.begin(), buffers.end(), [ecs](entt::entity a, entt::entity b) {
			const auto& ca = ecs->getComponent<rigkit::ecs::CCode>(a);
			const auto& cb = ecs->getComponent<rigkit::ecs::CCode>(b);
			if (ca.order != cb.order) {
				return ca.order < cb.order;
			}
			return ca.name < cb.name;
		});

		if (m_entity == entt::null && !buffers.empty()) {
			m_entity = buffers.front();
		}

		int current = 0;
		for (int i = 0; i < static_cast<int>(buffers.size()); ++i) {
			if (buffers[static_cast<size_t>(i)] == m_entity) {
				current = i;
				break;
			}
		}
		if (!buffers.empty()) {
			std::vector<std::string> labels;
			labels.reserve(buffers.size());
			for (auto e : buffers) {
				labels.push_back(ecs->getComponent<rigkit::ecs::CCode>(e).name);
			}
			if (ImGui::BeginCombo("Buffer", labels[static_cast<size_t>(current)].c_str())) {
				for (int i = 0; i < static_cast<int>(buffers.size()); ++i) {
					const bool selected = i == current;
					if (ImGui::Selectable(labels[static_cast<size_t>(i)].c_str(), selected)) {
						m_entity = buffers[static_cast<size_t>(i)];
						m_seenEpoch = 0;
						m_seenDirty = false;
						m_seenTextHash.clear();
						m_debounce = 0.3f; // compile soon
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
	}

	if (ImGui::Button("Compile")) {
		compileNow();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto", &m_autoCompile);
	ImGui::SameLine();
	bool paused = m_preview.paused();
	if (ImGui::Checkbox("Pause", &paused)) {
		m_preview.setPaused(paused);
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset time")) {
		m_preview.resetTime();
	}

	float speed = m_preview.speed();
	ImGui::SetNextItemWidth(120.f);
	if (ImGui::SliderFloat("Speed", &speed, 0.1f, 4.f, "%.1fx")) {
		m_preview.setSpeed(speed);
	}
	ImGui::SameLine();
	ImGui::Text("FPS %.0f", static_cast<double>(m_preview.fps()));

	if (m_preview.hasError()) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.35f, 1.f));
		ImGui::TextWrapped("%s", m_preview.error().c_str());
		ImGui::PopStyleColor();
	} else if (m_preview.hasProgram()) {
		ImGui::TextUnformatted("Compiled OK");
	}

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const int tw = std::max(1, static_cast<int>(avail.x));
	const int th = std::max(1, static_cast<int>(avail.y));

	const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##shader_preview", avail);
	const bool hovered = ImGui::IsItemHovered();
	if (hovered) {
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const float mx = mouse.x - canvasPos.x;
		const float my = avail.y - (mouse.y - canvasPos.y); // GL bottom-left
		m_mouse.x = mx;
		m_mouse.y = my;
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (!m_mouseDown) {
				m_mouse.z = mx;
				m_mouse.w = my;
			}
			m_mouseDown = true;
		} else {
			m_mouseDown = false;
			m_mouse.z = 0.f;
			m_mouse.w = 0.f;
		}
	}

	m_preview.render(tw, th, m_preview.time(), m_mouse);

	if (m_preview.colorTexture() != 0) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(m_preview.colorTexture())),
					 canvasPos, ImVec2(canvasPos.x + avail.x, canvasPos.y + avail.y));
	}
}
