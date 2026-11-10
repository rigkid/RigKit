#include <doctest.h>

#include "CDrawStyle.h"
#include "CRectangle.h"
#include "CStar.h"
#include "CTransform.h"
#include "CUiPanel.h"
#include "fixture.h"
#include "rig/create.h"

using namespace rigkit;
using namespace rigkit::smoke;

namespace {

const ComponentTypeInfo* requireType(SpineFixture& f, const char* name, bool portable) {
	const auto* info = f.findType(name);
	REQUIRE(info != nullptr);
	CHECK(info->portable == portable);
	return info;
}

template <typename T>
void checkPropertiesPointIntoMember(MEcs& ecs, const ComponentTypeInfo& info) {
	auto entity = ecs.createEntity();
	auto& comp = ecs.addComponent<T>(entity);
	auto props = ecs.registeredProperties(info, entity);
	REQUIRE_FALSE(props.empty());
	for (const auto& p : props) {
		CHECK_FALSE(p.name.empty());
		CHECK(propPointsIntoComponent(p, &comp, sizeof(T)));
	}
}

} // namespace

TEST_CASE("rigComponent registers portable spine types") {
	SpineFixture f;
	requireType(f, "Transform", true);
	requireType(f, "Relationship", true);
	requireType(f, "Rectangle", true);
	requireType(f, "Ellipse", true);
	requireType(f, "Line", true);
	requireType(f, "Polygon", true);
	requireType(f, "RegularPolygon", true);
	requireType(f, "Star", true);
	requireType(f, "Arc", true);
	requireType(f, "Ring", true);
	requireType(f, "Mesh", true);
	requireType(f, "DrawStyle", true);
	requireType(f, "Selection", true);
	requireType(f, "Guide", true);
	requireType(f, "Code", true);
	requireType(f, "Text", true);
	requireType(f, "Canvas", false);
	requireType(f, "UiPanel", true);
	requireType(f, "UiGroup", true);
	requireType(f, "UiControl", true);
	requireType(f, "UiAction", true);
}

TEST_CASE("rigComponent GetProperties pointers land in component storage") {
	SpineFixture f;
	auto& ecs = f.ecs();

	const auto* transform = requireType(f, "Transform", true);
	checkPropertiesPointIntoMember<ecs::CTransform>(ecs, *transform);

	// One box-shaped and one radial primitive, since they lay their fields out
	// differently and a reflection slip would only show on one of them.
	const auto* rectangle = requireType(f, "Rectangle", true);
	checkPropertiesPointIntoMember<ecs::CRectangle>(ecs, *rectangle);

	const auto* star = requireType(f, "Star", true);
	checkPropertiesPointIntoMember<ecs::CStar>(ecs, *star);

	const auto* drawStyle = requireType(f, "DrawStyle", true);
	checkPropertiesPointIntoMember<ecs::CDrawStyle>(ecs, *drawStyle);

	const auto* uiPanel = requireType(f, "UiPanel", true);
	checkPropertiesPointIntoMember<ecs::CUiPanel>(ecs, *uiPanel);

	const auto* uiAction = requireType(f, "UiAction", true);
	checkPropertiesPointIntoMember<ecs::CUiAction>(ecs, *uiAction);
}

TEST_CASE("rig::makeUi* writes portable panel data") {
	SpineFixture f;
	auto target = f.ecs().createEntity("knob");
	auto panel = rig::makeUiPanel(f.ecs(), "mod.lfo", ecs::CUiPanel::Surface::Hud, "LFO");
	auto ctrl = rig::makeUiControl(f.ecs(), panel, target, "frequency", 0,
								   ecs::CUiControl::Widget::Slider, "Rate");
	auto act = rig::makeUiAction(f.ecs(), panel, "lfo.resetPhase", 1, "Reset");

	REQUIRE(f.ecs().hasComponent<ecs::CUiPanel>(panel));
	CHECK(f.ecs().getComponent<ecs::CUiPanel>(panel).role == "mod.lfo");
	CHECK(f.ecs().getComponent<ecs::CUiPanel>(panel).surface == ecs::CUiPanel::Surface::Hud);

	REQUIRE(f.ecs().hasComponent<ecs::CUiControl>(ctrl));
	CHECK(f.ecs().getComponent<ecs::CUiControl>(ctrl).panel == entt::to_integral(panel));
	CHECK(f.ecs().getComponent<ecs::CUiControl>(ctrl).target == entt::to_integral(target));
	CHECK(f.ecs().getComponent<ecs::CUiControl>(ctrl).propertyKey == "frequency");
	CHECK(f.ecs().getComponent<ecs::CUiControl>(ctrl).widget == ecs::CUiControl::Widget::Slider);

	REQUIRE(f.ecs().hasComponent<ecs::CUiAction>(act));
	CHECK(f.ecs().getComponent<ecs::CUiAction>(act).actionId == "lfo.resetPhase");
}
