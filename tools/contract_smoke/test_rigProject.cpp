#include <doctest.h>

#include <filesystem>
#include <memory>

#include "CProject.h"
#include "CPage.h"
#include "CRelationship.h"
#include "CSelection.h"
#include "CShape.h"
#include "CTransform.h"
#include "ProjectSerializer.h"
#include "fixture.h"
#include "rig/create.h"

using namespace rigkit;
using namespace rigkit::smoke;

namespace {

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

std::filesystem::path tempRigPath(const char* name) {
	return std::filesystem::temp_directory_path() / name;
}

void buildRoundTripScene(MEcs& ecs, const std::string& docPath) {
	auto docEntity = ecs.createEntity("show-doc");
	ecs::CProject doc;
	doc.title = "Install Cue";
	doc.path = docPath;
	doc.activePageIndex = 1;
	doc.defaultUnit = "px";
	doc.author = "contract_smoke";
	doc.dirty = true;
	ecs.addComponent<ecs::CProject>(docEntity, doc);

	ecs::CPage pageA;
	pageA.name = "Open";
	pageA.index = 0;
	pageA.width = 1920.f;
	pageA.height = 1080.f;
	ecs.addComponent<ecs::CPage>(ecs.createEntity("page-Open"), pageA);

	ecs::CPage pageB;
	pageB.name = "Look";
	pageB.index = 1;
	pageB.width = 1280.f;
	pageB.height = 720.f;
	ecs.addComponent<ecs::CPage>(ecs.createEntity("page-Look"), pageB);

	auto parent = rig::makeRect(ecs, 100.f, 200.f, 220.f, 280.f, rig::fill(0.30f, 0.55f, 0.90f),
								"card-Open");
	auto child = rig::makeRect(ecs, 40.f, 30.f, 70.f, 50.f, rig::fill(0.95f, 0.55f, 0.25f),
							   "card-child");
	ecs::CRelationship rel;
	rel.parent = parent;
	ecs.addComponent<ecs::CRelationship>(child, rel);

	// Session-only — must not survive the round-trip.
	ecs::CSelection sel;
	sel.isSelected = true;
	ecs.addComponent<ecs::CSelection>(parent, sel);
}

void expectRoundTripScene(MEcs& ecs, const std::string& docPath) {
	auto docView = ecs.view<ecs::CProject>();
	REQUIRE(docView.begin() != docView.end());
	const auto& doc = docView.get<ecs::CProject>(*docView.begin());
	CHECK(doc.title == "Install Cue");
	CHECK(doc.activePageIndex == 1);
	CHECK(doc.defaultUnit == "px");
	CHECK(doc.author == "contract_smoke");
	CHECK(doc.path == docPath);
	CHECK_FALSE(doc.dirty);

	auto openPage = ecs.findEntity("page-Open");
	auto lookPage = ecs.findEntity("page-Look");
	REQUIRE(openPage != entt::null);
	REQUIRE(lookPage != entt::null);
	REQUIRE(ecs.hasComponent<ecs::CPage>(openPage));
	REQUIRE(ecs.hasComponent<ecs::CPage>(lookPage));
	CHECK(ecs.getComponent<ecs::CPage>(openPage).name == "Open");
	CHECK(ecs.getComponent<ecs::CPage>(lookPage).width == doctest::Approx(1280.f));

	auto parent = ecs.findEntity("card-Open");
	auto child = ecs.findEntity("card-child");
	REQUIRE(parent != entt::null);
	REQUIRE(child != entt::null);
	REQUIRE(ecs.hasComponent<ecs::CTransform>(parent));
	REQUIRE(ecs.hasComponent<ecs::CShape>(parent));
	CHECK(ecs.getComponent<ecs::CTransform>(parent).position.x == doctest::Approx(100.f));
	CHECK(ecs.getComponent<ecs::CShape>(parent).getWidth() == doctest::Approx(220.f));

	REQUIRE(ecs.hasComponent<ecs::CRelationship>(child));
	CHECK(ecs.getComponent<ecs::CRelationship>(child).parent == parent);

	// Selection is session state and must not be serialized.
	CHECK_FALSE(ecs.hasComponent<ecs::CSelection>(parent));
}

} // namespace

TEST_CASE("rigProject registers Project/Page and ProjectLoadSave system") {
	SpineFixture f;
	auto& ecs = f.ecs();

	const auto* doc = f.findType("Project");
	const auto* page = f.findType("Page");
	REQUIRE(doc != nullptr);
	REQUIRE(page != nullptr);
	CHECK(doc->portable);
	CHECK(page->portable);

	REQUIRE(ecs.hasSystem("ProjectLoadSave"));
	REQUIRE(f.findType("Transform") != nullptr);

	checkPropertiesPointIntoMember<ecs::CProject>(ecs, *doc);
	checkPropertiesPointIntoMember<ecs::CPage>(ecs, *page);
}

TEST_CASE("rigProject .rig serializer round-trips core components") {
	SpineFixture f;
	auto& ecs = f.ecs();
	const auto path = tempRigPath("rigkit_contract_smoke_roundtrip.rig").string();
	std::filesystem::remove(path);

	buildRoundTripScene(ecs, path);

	project::ProjectSerializer serializer;
	REQUIRE(serializer.save(ecs, path));
	REQUIRE(std::filesystem::exists(path));

	REQUIRE(serializer.load(ecs, path));
	expectRoundTripScene(ecs, path);

	std::filesystem::remove(path);
}

TEST_CASE("rigProject pack requestSave/requestLoad round-trips via Update") {
	SpineFixture f;
	auto& ecs = f.ecs();
	auto* packs = f.engine->getPackManager();
	REQUIRE(packs != nullptr);
	auto docPack = packs->getPack<rigProject>();
	REQUIRE(docPack != nullptr);

	const auto path = tempRigPath("rigkit_contract_smoke_pack_roundtrip.rig").string();
	std::filesystem::remove(path);

	buildRoundTripScene(ecs, path);
	docPack->requestSave(path);
	ecs.updateSystems(0.f);
	REQUIRE(std::filesystem::exists(path));

	docPack->requestLoad(path);
	ecs.updateSystems(0.f);
	expectRoundTripScene(ecs, path);

	std::filesystem::remove(path);
}
