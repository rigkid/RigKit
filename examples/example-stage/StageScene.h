#pragma once

#include <glm/glm.hpp>
#include <string>

#include "ecs/MEcs.h"

namespace stage {

/** @brief Announce `CStagePrefab` and `CStageScript` on the inspector catalog. */
void registerComponents(rigkit::MEcs& ecs);

/**
 * @brief Feature scene actors: world, ground, wind, cloth, ball, camera, light,
 * drop prefab, play script.
 * @details Does not import a mesh. Call `addProp` for the glTF prop.
 */
void addFeatureActors(rigkit::MEcs& ecs);

/** @brief Named entity with a `CTransform` at the origin. */
entt::entity create(rigkit::MEcs& ecs, const std::string& name);
void destroy(rigkit::MEcs& ecs, entt::entity entity);
entt::entity find(rigkit::MEcs& ecs, const std::string& name);

/**
 * @brief True when **rigJolt** writes this entity's pose each step.
 * @details Dynamic spheres and cloth. A marker, camera, light, or static mesh is false.
 */
bool simOwnsPose(rigkit::MEcs& ecs, entt::entity entity);

bool positionOf(rigkit::MEcs& ecs, entt::entity entity, glm::vec3& out);

/**
 * @brief Write `CTransform::position`.
 * @return false when @p entity is missing, has no transform, or `simOwnsPose`.
 */
bool setPosition(rigkit::MEcs& ecs, entt::entity entity, glm::vec3 position);

/** @brief `CRigidbody::velocity` (launch velocity, not the live solver speed). */
bool velocityOf(rigkit::MEcs& ecs, entt::entity entity, glm::vec3& out);

/**
 * @brief Spawn a dynamic sphere from a `CStagePrefab` name.
 * @return `entt::null` when no recipe uses @p prefab.
 */
entt::entity spawn(rigkit::MEcs& ecs, const std::string& prefab, glm::vec3 at, glm::vec3 velocity);

int rigidbodyCount(rigkit::MEcs& ecs);

} // namespace stage
