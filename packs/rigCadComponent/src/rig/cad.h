#pragma once

/**
 * @file
 * @brief Creators for CAD solids and datums.
 */

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include "CCadBoolean.h"
#include "CCadDimension.h"
#include "rig/create.h"

namespace rigkit {
class MEcs;
}

namespace rig {

entt::entity makeCadBox(float sizeX, float sizeY, float sizeZ, bool center = true,
						const std::string& name = {});
entt::entity makeCadBox(rigkit::MEcs& ecs, float sizeX, float sizeY, float sizeZ,
						bool center = true, const std::string& name = {});

entt::entity makeCadCylinder(float radius, float height, bool center = true,
							 const std::string& name = {});
entt::entity makeCadCylinder(rigkit::MEcs& ecs, float radius, float height, bool center = true,
							 const std::string& name = {});

entt::entity makeCadSphere(float radius, const std::string& name = {});
entt::entity makeCadSphere(rigkit::MEcs& ecs, float radius, const std::string& name = {});

entt::entity makeCadBoolean(rigkit::ecs::CCadBoolean::Op op, std::vector<std::string> operands,
							const rigkit::ecs::CDrawStyle& style = fill(1, 1, 1),
							const std::string& name = {});
entt::entity makeCadBoolean(rigkit::MEcs& ecs, rigkit::ecs::CCadBoolean::Op op,
							std::vector<std::string> operands,
							const rigkit::ecs::CDrawStyle& style = fill(1, 1, 1),
							const std::string& name = {});

entt::entity makeCadDimension(rigkit::ecs::CCadDimension::Kind kind, const std::string& a,
							  const std::string& b, float value, bool measurement = false,
							  const std::string& name = {});
entt::entity makeCadDimension(rigkit::MEcs& ecs, rigkit::ecs::CCadDimension::Kind kind,
							  const std::string& a, const std::string& b, float value,
							  bool measurement = false, const std::string& name = {});

entt::entity makeCadReferenceLine(glm::vec3 origin, glm::vec3 direction,
								  const std::string& name = {});
entt::entity makeCadReferenceLine(rigkit::MEcs& ecs, glm::vec3 origin, glm::vec3 direction,
								  const std::string& name = {});

entt::entity makeCadReferenceLineThrough(const std::string& a, const std::string& b,
										 const std::string& name = {});
entt::entity makeCadReferenceLineThrough(rigkit::MEcs& ecs, const std::string& a,
										 const std::string& b, const std::string& name = {});

entt::entity makeCadReferencePlane(glm::vec3 origin, glm::vec3 normal,
								   const std::string& name = {});
entt::entity makeCadReferencePlane(rigkit::MEcs& ecs, glm::vec3 origin, glm::vec3 normal,
								   const std::string& name = {});

entt::entity makeCadReferencePlaneThrough(const std::string& a, glm::vec3 normal,
										  const std::string& name = {});
entt::entity makeCadReferencePlaneThrough(rigkit::MEcs& ecs, const std::string& a, glm::vec3 normal,
										  const std::string& name = {});

} // namespace rig
