#include "ecs/MEcs.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace rigkit {

void warnComponentHasNoProperties(const std::string& name) {
	spdlog::warn("[Property Inspector] component '{}' has no GetProperties() — no fields shown. "
				 "Add one, or declare kNoProperties to opt out.",
				 name);
}

MEcs::MEcs() {
	m_eventSystem = std::make_unique<rigkit::ecs::SEvent>();
}

MEcs::~MEcs() {
	clear();
}

entt::entity MEcs::createEntity(const std::string& name) {
	auto entity = m_registry.create();
	m_entities.push_back(entity);

	if (!name.empty()) {
		m_namedEntities[name] = entity;
	}

	return entity;
}

void MEcs::destroyEntity(entt::entity entity) {
	for (auto it = m_namedEntities.begin(); it != m_namedEntities.end(); ++it) {
		if (it->second == entity) {
			m_namedEntities.erase(it);
			break;
		}
	}

	auto it = std::find(m_entities.begin(), m_entities.end(), entity);
	if (it != m_entities.end()) {
		m_entities.erase(it);
	}

	m_registry.destroy(entity);
}

entt::entity MEcs::findEntity(const std::string& name) {
	auto it = m_namedEntities.find(name);
	if (it != m_namedEntities.end()) {
		return it->second;
	}
	return entt::null;
}

std::string MEcs::entityName(entt::entity entity) const {
	for (const auto& [name, ent] : m_namedEntities) {
		if (ent == entity) {
			return name;
		}
	}
	return {};
}

bool MEcs::hasRegisteredComponent(const ComponentTypeInfo& info, entt::entity entity) const {
	return info.has && info.has(const_cast<entt::registry&>(m_registry), entity);
}

std::vector<sProp> MEcs::registeredProperties(const ComponentTypeInfo& info,
											  entt::entity entity) const {
	if (!info.properties) {
		return {};
	}
	return info.properties(const_cast<entt::registry&>(m_registry), entity);
}

void MEcs::storeSystem(const std::string& name, SystemPhase phase, SystemFn fn) {
	for (auto& entry : m_systems) {
		if (entry.name == name && entry.phase == phase) {
			entry.fn = std::move(fn);
			return;
		}
	}
	m_systems.push_back(SystemEntry{name, phase, std::move(fn)});
}

bool MEcs::hasSystem(const std::string& name) const {
	for (const auto& entry : m_systems) {
		if (entry.name == name) {
			return true;
		}
	}
	return false;
}

void MEcs::runSystems(SystemPhase phase, float dt) {
	for (auto& entry : m_systems) {
		if (entry.phase == phase && entry.fn) {
			entry.fn(dt);
		}
	}
}

void MEcs::updateSystems(float deltaTime) {
	// Host event bus always runs; packs register the rest via SystemRegistry.
	if (m_eventSystem) {
		m_eventSystem->update();
	}
	runSystems(SystemPhase::Update, deltaTime);
}

void MEcs::renderSystems() {
	if (m_inDrawPhase) {
		return;
	}
	m_inDrawPhase = true;
	runSystems(SystemPhase::Draw, 0.0f);
	m_inDrawPhase = false;
}

void MEcs::clear() {
	m_registry.clear();
	m_namedEntities.clear();
	m_entities.clear();
}

size_t MEcs::getEntityCount() const {
	return m_entities.size();
}

std::vector<entt::entity> MEcs::getAllEntities() const {
	return m_entities;
}

rigkit::json MEcs::getSettings() const {
	rigkit::json j;
	j["entities"] = rigkit::json::array();
	for (auto entity : m_entities) {
		rigkit::json entityJson;
		entityJson["id"] = static_cast<uint32_t>(entity);
		for (const auto& [name, ent] : m_namedEntities) {
			if (ent == entity) {
				entityJson["name"] = name;
				break;
			}
		}
		j["entities"].push_back(entityJson);
	}
	return j;
}

void MEcs::setSettings(const rigkit::json& settings) {
	clear();
	if (!settings.contains("entities"))
		return;
	for (const auto& entityJson : settings["entities"]) {
		std::string name = entityJson.value("name", "");
		createEntity(name);
	}
}

} // namespace rigkit
