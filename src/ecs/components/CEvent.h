#pragma once

#include <any>
#include <entt/entt.hpp>
#include <string>

namespace rigkit {
namespace ecs {

// Event types
enum class EET_EventType {
	MenuAction,
	WindowAction,
	ThemeAction,
	WorkspaceAction,
	CanvasAction,
	DebugAction,
	Custom
};

struct CEvent {
	EET_EventType type;
	std::string actionId;
	std::any data;
	entt::entity source = entt::null;
	entt::entity target = entt::null;

	CEvent(EET_EventType t, const std::string& id) : type(t), actionId(id) {}
	CEvent(EET_EventType t, const std::string& id, const std::any& d)
		: type(t), actionId(id), data(d) {}
};

} // namespace ecs
} // namespace rigkit
