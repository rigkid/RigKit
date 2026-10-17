#include "rig/cad.h"

#include "ecs/MEcs.h"
#include "CCadBox.h"
#include "CCadCylinder.h"
#include "CCadReferenceLine.h"
#include "CCadReferencePlane.h"
#include "CCadSphere.h"

namespace rig {

entt::entity makeCadBox(rigkit::MEcs& ecs, float sizeX, float sizeY, float sizeZ, bool center,
						const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadBox box;
	box.sizeX = sizeX;
	box.sizeY = sizeY;
	box.sizeZ = sizeZ;
	box.center = center;
	ecs.addComponent(e, box);
	return e;
}

entt::entity makeCadBox(float sizeX, float sizeY, float sizeZ, bool center,
						const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadBox(*ecs, sizeX, sizeY, sizeZ, center, name) : entt::null;
}

entt::entity makeCadCylinder(rigkit::MEcs& ecs, float radius, float height, bool center,
							 const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadCylinder cyl;
	cyl.radius = radius;
	cyl.height = height;
	cyl.center = center;
	ecs.addComponent(e, cyl);
	return e;
}

entt::entity makeCadCylinder(float radius, float height, bool center, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadCylinder(*ecs, radius, height, center, name) : entt::null;
}

entt::entity makeCadSphere(rigkit::MEcs& ecs, float radius, const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadSphere sphere;
	sphere.radius = radius;
	ecs.addComponent(e, sphere);
	return e;
}

entt::entity makeCadSphere(float radius, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadSphere(*ecs, radius, name) : entt::null;
}

entt::entity makeCadBoolean(rigkit::MEcs& ecs, rigkit::ecs::CCadBoolean::Op op,
							std::vector<std::string> operands, const rigkit::ecs::CDrawStyle& style,
							const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	ecs.addComponent(e, style);
	rigkit::ecs::CCadBoolean b;
	b.op = op;
	b.operands = std::move(operands);
	ecs.addComponent(e, std::move(b));
	return e;
}

entt::entity makeCadBoolean(rigkit::ecs::CCadBoolean::Op op, std::vector<std::string> operands,
							const rigkit::ecs::CDrawStyle& style, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadBoolean(*ecs, op, std::move(operands), style, name) : entt::null;
}

entt::entity makeCadDimension(rigkit::MEcs& ecs, rigkit::ecs::CCadDimension::Kind kind,
							  const std::string& a, const std::string& b, float value,
							  bool measurement, const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadDimension dim;
	dim.kind = kind;
	dim.a = a;
	dim.b = b;
	dim.value = value;
	dim.measurement = measurement;
	ecs.addComponent(e, dim);
	return e;
}

entt::entity makeCadDimension(rigkit::ecs::CCadDimension::Kind kind, const std::string& a,
							  const std::string& b, float value, bool measurement,
							  const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadDimension(*ecs, kind, a, b, value, measurement, name) : entt::null;
}

entt::entity makeCadReferenceLine(rigkit::MEcs& ecs, glm::vec3 origin, glm::vec3 direction,
								  const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadReferenceLine line;
	line.origin = origin;
	line.direction = direction;
	ecs.addComponent(e, line);
	return e;
}

entt::entity makeCadReferenceLine(glm::vec3 origin, glm::vec3 direction, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadReferenceLine(*ecs, origin, direction, name) : entt::null;
}

entt::entity makeCadReferenceLineThrough(rigkit::MEcs& ecs, const std::string& a,
										 const std::string& b, const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadReferenceLine line;
	line.a = a;
	line.b = b;
	ecs.addComponent(e, line);
	return e;
}

entt::entity makeCadReferenceLineThrough(const std::string& a, const std::string& b,
										 const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadReferenceLineThrough(*ecs, a, b, name) : entt::null;
}

entt::entity makeCadReferencePlane(rigkit::MEcs& ecs, glm::vec3 origin, glm::vec3 normal,
								   const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadReferencePlane plane;
	plane.origin = origin;
	plane.normal = normal;
	ecs.addComponent(e, plane);
	return e;
}

entt::entity makeCadReferencePlane(glm::vec3 origin, glm::vec3 normal, const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadReferencePlane(*ecs, origin, normal, name) : entt::null;
}

entt::entity makeCadReferencePlaneThrough(rigkit::MEcs& ecs, const std::string& a, glm::vec3 normal,
										  const std::string& name) {
	auto e = ecs.createEntity(name);
	ecs.addComponent(e, rigkit::ecs::CTransform{});
	rigkit::ecs::CCadReferencePlane plane;
	plane.a = a;
	plane.normal = normal;
	ecs.addComponent(e, plane);
	return e;
}

entt::entity makeCadReferencePlaneThrough(const std::string& a, glm::vec3 normal,
										  const std::string& name) {
	auto* ecs = currentEcs();
	return ecs ? makeCadReferencePlaneThrough(*ecs, a, normal, name) : entt::null;
}

} // namespace rig
