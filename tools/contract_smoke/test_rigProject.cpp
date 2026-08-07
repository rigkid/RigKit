#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include "CProject.h"
#include "CPage.h"
#include "CCamera.h"
#include "CCode.h"
#include "CDrawStyle.h"
#include "CLight.h"
#include "CMesh.h"
#include "CRelationship.h"
#include "CSelection.h"
#include "CRectangle.h"
#include "CTransform.h"
#include "ContractImport.h"
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

std::string readFileText(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
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
	REQUIRE(ecs.hasComponent<ecs::CRectangle>(parent));
	CHECK(ecs.getComponent<ecs::CTransform>(parent).position.x == doctest::Approx(100.f));
	CHECK(ecs.getComponent<ecs::CRectangle>(parent).width == doctest::Approx(220.f));

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

TEST_CASE("rigProject Contract import maps rig.media.code to CCode") {
	SpineFixture f;
	auto& ecs = f.ecs();
	const char* jsonText = R"({
		"rig": "0.9.0",
		"document": { "title": "code-smoke" },
		"entities": [
			{
				"id": "gradient",
				"components": {
					"rig.meta.named": { "name": "gradient" },
					"rig.media.code": {
						"language": "glsl",
						"text": "void mainImage(out vec4 c, in vec2 p) { c = vec4(1.0); }"
					}
				}
			}
		]
	})";
	auto result = project::importContractJson(ecs, jsonText, "smoke");
	REQUIRE(result.ok);
	CHECK(result.skipped.empty());
	auto view = ecs.view<ecs::CCode>();
	REQUIRE(view.begin() != view.end());
	const auto& code = view.get<ecs::CCode>(*view.begin());
	CHECK(code.language == "glsl");
	CHECK(code.name == "gradient");
	CHECK(code.text.find("mainImage") != std::string::npos);
}

TEST_CASE("rigProject Contract import maps rig.* geometry light material") {
	SpineFixture f;
	auto& ecs = f.ecs();
	const char* jsonText = R"({
		"rig": "0.9.0",
		"document": { "title": "smoke-contract" },
		"entities": [
			{
				"id": "cam",
				"components": {
					"rig.spatial.transform": { "position": [0, 0, 5] },
					"rig.spatial.camera": { "active": true, "projection": "perspective" }
				}
			},
			{
				"id": "sun",
				"components": {
					"rig.spatial.transform": { "position": [2, 4, 1] },
					"rig.render.light": {
						"enabled": true,
						"type": "directional",
						"rgb": [1, 1, 1],
						"intensity": 1,
						"ambient": 0.3
					}
				}
			},
			{
				"id": "box",
				"components": {
					"rig.spatial.transform": { "position": [0, 0, 0] },
					"rig.geometry.mesh": {
						"mode": "triangles",
						"positions": [0,0,0, 1,0,0, 0,1,0],
						"indices": [0, 1, 2]
					},
					"rig.render.material": { "albedoRgb": [0.2, 0.4, 0.8] }
				}
			}
		]
	})";

	auto result = project::importContractJson(ecs, jsonText, "smoke");
	REQUIRE(result.ok);
	CHECK(result.title == "smoke-contract");
	CHECK(result.skipped.empty());
	CHECK(result.geometryCount == 1);
	CHECK(result.entityCount == 3);

	auto lightView = ecs.view<ecs::CLight>();
	REQUIRE(lightView.begin() != lightView.end());

	auto meshView = ecs.view<ecs::CMesh, ecs::CDrawStyle>();
	REQUIRE(meshView.begin() != meshView.end());
	const auto& style = meshView.get<ecs::CDrawStyle>(*meshView.begin());
	CHECK(style.hasFill);
	CHECK(style.fillR == doctest::Approx(0.2f));
	CHECK(style.fillB == doctest::Approx(0.8f));

	auto camView = ecs.view<ecs::CCamera>();
	REQUIRE(camView.begin() != camView.end());
}

/**
 * The writer and the Contract reader must agree on one format. Every other test
 * here checks a side against itself — the serializer against its own loader, the
 * importer against a hand-written literal — so both stay green while producing
 * files the other cannot read. This is the only case that joins them.
 */
TEST_CASE("rigProject .rig save is readable by Contract import") {
	const auto path = tempRigPath("rigkit_contract_smoke_writer_to_reader.rig").string();
	std::filesystem::remove(path);

	{
		SpineFixture writer;
		buildRoundTripScene(writer.ecs(), path);
		project::ProjectSerializer serializer;
		REQUIRE(serializer.save(writer.ecs(), path));
	}

	const std::string text = readFileText(path);
	REQUIRE_FALSE(text.empty());

	SpineFixture reader;
	auto& ecs = reader.ecs();
	auto result = project::importContractJson(ecs, text, path);

	// Without this the failure reads as a bare REQUIRE(false) and hides which
	// side broke — a parse error, a missing envelope, or an unreadable key.
	INFO("import error: " << result.error);
	// A fatal assert skips the cleanup below, so the file stays for inspection.
	INFO("written file: " << path);
	REQUIRE(result.ok);
	// Extensions are reported as skipped by design, so only a rig.* id the
	// reader does not know is a fault — that means the writer invented one.
	for (const auto& key : result.skipped) {
		INFO("unknown component key: " << key);
		CHECK(key.rfind("rig.", 0) != 0);
	}
	// Guards against an envelope that parses but carries nothing.
	CHECK(result.geometryCount >= 1);

	// Parenting only rebuilds if entity ids survived as Contract string ids.
	bool reparented = false;
	for (auto e : ecs.view<ecs::CRelationship>()) {
		if (ecs.getComponent<ecs::CRelationship>(e).parent != entt::null) {
			reparented = true;
		}
	}
	CHECK(reparented);

	std::filesystem::remove(path);
}
