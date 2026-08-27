#include <doctest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include "CAssetRef.h"
#include "CCamera.h"
#include "CCode.h"
#include "CCurve.h"
#include "CDrawStyle.h"
#include "CLayer.h"
#include "CLight.h"
#include "CMesh.h"
#include "CPage.h"
#include "CPath.h"
#include "CProject.h"
#include "CRectangle.h"
#include "CRelationship.h"
#include "CSelection.h"
#include "CText.h"
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
	pageB.originAnchor = 4;
	ecs.addComponent<ecs::CPage>(ecs.createEntity("page-Look"), pageB);

	auto parent =
		rig::makeRect(ecs, 100.f, 200.f, 220.f, 280.f, rig::fill(0.30f, 0.55f, 0.90f), "card-Open");
	auto child =
		rig::makeRect(ecs, 40.f, 30.f, 70.f, 50.f, rig::fill(0.95f, 0.55f, 0.25f), "card-child");
	ecs::CRelationship rel;
	rel.parent = parent;
	ecs.addComponent<ecs::CRelationship>(child, rel);

	// Session-only - must not survive the round-trip.
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
	CHECK(ecs.entityName(openPage) == "page-Open");
	CHECK(ecs.getComponent<ecs::CPage>(lookPage).width == doctest::Approx(1280.f));
	CHECK(ecs.getComponent<ecs::CPage>(lookPage).originAnchor == 4);

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

project::ContractImportResult importWithPackCodecs(SpineFixture& f, const char* jsonText,
												   const char* label) {
	auto* packs = f.engine->getPackManager();
	REQUIRE(packs != nullptr);
	auto docPack = packs->getPack<rigProject>();
	REQUIRE(docPack != nullptr);
	return project::importContractJson(f.ecs(), jsonText, label, docPack->serializer().registry());
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
	auto* packs = f.engine->getPackManager();
	REQUIRE(packs != nullptr);
	auto docPack = packs->getPack<rigProject>();
	REQUIRE(docPack != nullptr);

	const auto path = tempRigPath("rigkit_contract_smoke_roundtrip.rig").string();
	std::filesystem::remove(path);

	buildRoundTripScene(ecs, path);

	REQUIRE(docPack->serializer().save(ecs, path));
	REQUIRE(std::filesystem::exists(path));

	REQUIRE(docPack->serializer().load(ecs, path));
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

TEST_CASE("rigProject Contract import maps rig.spatial.anchor onto the page") {
	SpineFixture f;
	auto& ecs = f.ecs();
	const char* jsonText = R"({
		"rig": "0.13.0",
		"document": { "title": "anchor-smoke" },
		"entities": [
			{
				"id": "sheet",
				"components": {
					"rig.layout.page": { "width": 210, "height": 297 },
					"rig.spatial.anchor": { "point": "bottom-left" }
				}
			}
		]
	})";
	auto result = importWithPackCodecs(f, jsonText, "smoke");
	REQUIRE(result.ok);
	CHECK(result.skipped.empty());
	auto view = ecs.view<ecs::CPage>();
	REQUIRE(view.begin() != view.end());
	CHECK(view.get<ecs::CPage>(*view.begin()).originAnchor == 6);
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
	auto result = importWithPackCodecs(f, jsonText, "smoke");
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

	auto result = importWithPackCodecs(f, jsonText, "smoke");
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
 * here checks a side against itself - the serializer against its own loader, the
 * importer against a hand-written literal - so both stay green while producing
 * files the other cannot read. This is the only case that joins them.
 */
TEST_CASE("rigProject .rig save is readable by Contract import") {
	const auto path = tempRigPath("rigkit_contract_smoke_writer_to_reader.rig").string();
	std::filesystem::remove(path);

	{
		SpineFixture writer;
		auto* packs = writer.engine->getPackManager();
		REQUIRE(packs != nullptr);
		auto docPack = packs->getPack<rigProject>();
		REQUIRE(docPack != nullptr);
		buildRoundTripScene(writer.ecs(), path);
		REQUIRE(docPack->serializer().save(writer.ecs(), path));
	}

	const std::string text = readFileText(path);
	REQUIRE_FALSE(text.empty());

	SpineFixture reader;
	auto& ecs = reader.ecs();
	auto result = importWithPackCodecs(reader, text.c_str(), path.c_str());

	// Without this the failure reads as a bare REQUIRE(false) and hides which
	// side broke - a parse error, a missing envelope, or an unreadable key.
	INFO("import error: " << result.error);
	// A fatal assert skips the cleanup below, so the file stays for inspection.
	INFO("written file: " << path);
	REQUIRE(result.ok);
	// Extensions are reported as skipped by design, so only a rig.* id the
	// reader does not know is a fault - that means the writer invented one.
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

TEST_CASE("rigProject round-trips path layer asset text curve via pack codecs") {
	SpineFixture f;
	auto& ecs = f.ecs();
	auto* packs = f.engine->getPackManager();
	REQUIRE(packs != nullptr);
	auto docPack = packs->getPack<rigProject>();
	REQUIRE(docPack != nullptr);

	const auto path = tempRigPath("rigkit_contract_smoke_close_codecs.rig").string();
	std::filesystem::remove(path);

	auto docEntity = ecs.createEntity("doc");
	ecs::CProject doc;
	doc.title = "Close Codecs";
	doc.path = path;
	ecs.addComponent<ecs::CProject>(docEntity, doc);

	auto font = ecs.createEntity("face");
	ecs::CAssetRef asset;
	asset.kind = ecs::CAssetRef::Kind::Font;
	asset.path = "fonts/Inter.ttf";
	ecs.addComponent<ecs::CAssetRef>(font, asset);

	auto label = ecs.createEntity("label");
	ecs::CText text;
	text.text = "Hello";
	text.font = font;
	text.fontSize = 18.f;
	text.useKerning = false;
	ecs.addComponent<ecs::CText>(label, text);

	auto layer = ecs.createEntity("plate");
	ecs::CLayer lay;
	lay.order = 3;
	lay.locked = true;
	lay.visible = false;
	lay.colorR = 0.2f;
	lay.colorG = 0.4f;
	lay.colorB = 0.6f;
	lay.colorA = 1.f;
	ecs.addComponent<ecs::CLayer>(layer, lay);

	auto stroke = ecs.createEntity("stroke");
	ecs::CPath pathPod;
	ecs::CPath::Command move;
	move.type = ecs::CPath::Cmd::MoveTo;
	move.p = {0.f, 0.f};
	ecs::CPath::Command line;
	line.type = ecs::CPath::Cmd::LineTo;
	line.p = {10.f, 20.f};
	pathPod.commands = {move, line};
	ecs.addComponent<ecs::CPath>(stroke, pathPod);

	auto ramp = ecs.createEntity("ramp");
	ecs::CCurve curve;
	curve.preset = ecs::CCurve::Preset::EaseInOut;
	curve::applyPreset(curve, curve.preset);
	curve.interp = ecs::CCurve::Interp::Smooth;
	ecs.addComponent<ecs::CCurve>(ramp, curve);

	REQUIRE(docPack->serializer().save(ecs, path));
	REQUIRE(docPack->serializer().load(ecs, path));

	auto face = ecs.findEntity("face");
	auto textE = ecs.findEntity("label");
	auto plate = ecs.findEntity("plate");
	auto strokeE = ecs.findEntity("stroke");
	auto rampE = ecs.findEntity("ramp");
	REQUIRE(face != entt::null);
	REQUIRE(textE != entt::null);
	REQUIRE(plate != entt::null);
	REQUIRE(strokeE != entt::null);
	REQUIRE(rampE != entt::null);

	REQUIRE(ecs.hasComponent<ecs::CAssetRef>(face));
	CHECK(ecs.getComponent<ecs::CAssetRef>(face).kind == ecs::CAssetRef::Kind::Font);
	CHECK(ecs.getComponent<ecs::CAssetRef>(face).path == "fonts/Inter.ttf");

	REQUIRE(ecs.hasComponent<ecs::CText>(textE));
	CHECK(ecs.getComponent<ecs::CText>(textE).text == "Hello");
	CHECK(ecs.getComponent<ecs::CText>(textE).font == face);
	CHECK(ecs.getComponent<ecs::CText>(textE).fontSize == doctest::Approx(18.f));
	CHECK_FALSE(ecs.getComponent<ecs::CText>(textE).useKerning);

	REQUIRE(ecs.hasComponent<ecs::CLayer>(plate));
	CHECK(ecs.getComponent<ecs::CLayer>(plate).order == 3);
	CHECK(ecs.getComponent<ecs::CLayer>(plate).locked);
	CHECK_FALSE(ecs.getComponent<ecs::CLayer>(plate).visible);
	CHECK(ecs.getComponent<ecs::CLayer>(plate).colorG == doctest::Approx(0.4f));

	REQUIRE(ecs.hasComponent<ecs::CPath>(strokeE));
	REQUIRE(ecs.getComponent<ecs::CPath>(strokeE).commands.size() == 2);
	CHECK(ecs.getComponent<ecs::CPath>(strokeE).commands[1].p.y == doctest::Approx(20.f));

	REQUIRE(ecs.hasComponent<ecs::CCurve>(rampE));
	CHECK(ecs.getComponent<ecs::CCurve>(rampE).preset == ecs::CCurve::Preset::EaseInOut);
	CHECK(ecs.getComponent<ecs::CCurve>(rampE).interp == ecs::CCurve::Interp::Smooth);

	std::filesystem::remove(path);
}
