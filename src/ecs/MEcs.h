#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/IManager.h"
#include "core/ISettings.h"
#include "ecs/ComponentCatalog.h"
#include "ecs/SystemRegistry.h"
#include "ecs/systems/SEvent.h"
#include "rendering/IRenderer.h"

namespace rigkit {
class MRendering;

/// Logged once per data type whose fields never reach the inspector. Out of
/// line so no header pulls a logger in behind it.
void warnComponentHasNoProperties(const std::string& name);

/**
 * @brief Host ECS wrapper: entities, component catalog, system registry.
 * @details Packs register data types and Update/Draw systems. Present uses a
 * non-owning IRenderer* set for the current Draw pass.
 * @see ComponentCatalog, SystemRegistry, rigComponent, rigSystems
 */
class MEcs : public IManager, public ISettings {
  public:
	MEcs();
	~MEcs();
	void init() override {}
	void shutdown() override {}

	entt::entity createEntity(const std::string& name = "");
	void destroyEntity(entt::entity entity);
	entt::entity findEntity(const std::string& name);

	template <typename T> T& addComponent(entt::entity entity, const T& component = T{});

	template <typename T> T& getComponent(entt::entity entity);

	template <typename T> bool hasComponent(entt::entity entity);

	template <typename T> void removeComponent(entt::entity entity);

	/** @brief Announce a data type for inspector / discovery (host glue). */
	template <typename T> void registerComponent(const std::string& name, bool portable = true);

	const std::vector<ComponentTypeInfo>& componentTypes() const { return m_componentCatalog; }

	bool hasRegisteredComponent(const ComponentTypeInfo& info, entt::entity entity) const;
	std::vector<sProp> registeredProperties(const ComponentTypeInfo& info,
											entt::entity entity) const;

	/**
	 * @brief Register a fulfillment system. Packs own the functions.
	 * @details Pass the system straight in — the manager binds itself, so a
	 * system takes only what it uses: `void(MEcs&, float)`, `void(MEcs&)`,
	 * `void(float)` or `void()`.
	 * @param name Identity for the entry: registering the same name and phase
	 *        replaces it. That keeps a pack idempotent, since MPack::reloadPack
	 *        runs setup() again and there is no unregister. A capturing lambda
	 *        is a fresh object each time, so only the name survives a re-run.
	 * @code
	 * ecs->registerSystem("SHierarchy", SystemPhase::Update, ecs::SHierarchy);
	 * @endcode
	 */
	template <typename Fn> void registerSystem(const std::string& name, SystemPhase phase, Fn&& fn);

	/** @brief True when a system of this name is registered in any phase. */
	bool hasSystem(const std::string& name) const;
	/** @brief Registered entries in run order (name + phase). */
	const std::vector<SystemEntry>& systems() const { return m_systems; }

	void runSystems(SystemPhase phase, float dt);

	void updateSystems(float deltaTime);
	/** @brief Run Draw-phase systems (re-entrancy guarded). */
	void renderSystems();

	template <typename... Components> auto view();

	void clear();
	size_t getEntityCount() const;
	std::vector<entt::entity> getAllEntities() const;

	/** @brief Underlying EnTT registry for pack serializers / systems. */
	entt::registry& registry() { return m_registry; }
	const entt::registry& registry() const { return m_registry; }

	/** @brief Name passed to createEntity, or empty if unnamed. */
	std::string entityName(entt::entity entity) const;

	void setRenderingManager(MRendering* renderingManager) {
		m_renderingManager = renderingManager;
	}
	MRendering* getRenderingManager() const { return m_renderingManager; }

	void setPresentRenderer(IRenderer* renderer) { m_presentRenderer = renderer; }
	IRenderer* getPresentRenderer() const { return m_presentRenderer; }

	ecs::SEvent& getEventSystem() { return *m_eventSystem; }
	const ecs::SEvent& getEventSystem() const { return *m_eventSystem; }

	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	/** @brief Store an already-bound system call (non-template part). */
	void storeSystem(const std::string& name, SystemPhase phase, SystemFn fn);

	entt::registry m_registry;
	std::unordered_map<std::string, entt::entity> m_namedEntities;
	std::vector<entt::entity> m_entities;

	MRendering* m_renderingManager = nullptr;
	IRenderer* m_presentRenderer = nullptr;
	std::unique_ptr<ecs::SEvent> m_eventSystem;

	std::vector<ComponentTypeInfo> m_componentCatalog;
	std::vector<SystemEntry> m_systems;
	bool m_inDrawPhase = false;
};

template <typename T> T& MEcs::addComponent(entt::entity entity, const T& component) {
	return m_registry.emplace<T>(entity, component);
}

template <typename T> T& MEcs::getComponent(entt::entity entity) {
	return m_registry.get<T>(entity);
}

template <typename T> bool MEcs::hasComponent(entt::entity entity) {
	return m_registry.all_of<T>(entity);
}

template <typename T> void MEcs::removeComponent(entt::entity entity) {
	m_registry.remove<T>(entity);
}

template <typename T> void MEcs::registerComponent(const std::string& name, bool portable) {
	for (const auto& existing : m_componentCatalog) {
		if (existing.name == name) {
			return;
		}
	}
	if constexpr (!has_get_properties<T>::value && !declares_no_properties<T>::value) {
		warnComponentHasNoProperties(name);
	}
	ComponentTypeInfo info;
	info.name = name;
	info.portable = portable;
	info.has = [](entt::registry& reg, entt::entity e) { return reg.all_of<T>(e); };
	info.properties = [](entt::registry& reg, entt::entity e) -> std::vector<sProp> {
		if (!reg.all_of<T>(e)) {
			return {};
		}
		return TryGetProperties(reg.get<T>(e));
	};
	m_componentCatalog.push_back(std::move(info));
}

template <typename Fn>
void MEcs::registerSystem(const std::string& name, SystemPhase phase, Fn&& fn) {
	if constexpr (std::is_invocable_v<Fn&, MEcs&, float>) {
		storeSystem(name, phase,
					[this, fn = std::forward<Fn>(fn)](float dt) mutable { fn(*this, dt); });
	} else if constexpr (std::is_invocable_v<Fn&, MEcs&>) {
		storeSystem(name, phase, [this, fn = std::forward<Fn>(fn)](float) mutable { fn(*this); });
	} else if constexpr (std::is_invocable_v<Fn&, float>) {
		storeSystem(name, phase, [fn = std::forward<Fn>(fn)](float dt) mutable { fn(dt); });
	} else {
		static_assert(std::is_invocable_v<Fn&>,
					  "system must be callable as void(MEcs&, float), void(MEcs&), "
					  "void(float) or void()");
		storeSystem(name, phase, [fn = std::forward<Fn>(fn)](float) mutable { fn(); });
	}
}

template <typename... Components> auto MEcs::view() {
	return m_registry.view<Components...>();
}
} // namespace rigkit
