#include "App.h"
#include "Hal.h"
#include "Json.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

int gFails = 0;

void check(bool ok, const char* text, int line) {
	if (ok) {
		return;
	}
	std::fprintf(stderr, "fail:%d %s\n", line, text);
	++gFails;
}

bool near(float a, float b) {
	return std::fabs(a - b) < 0.001f;
}

rigkit::json::Value* componentOf(rigkit::json::Value& doc, const char* schema) {
	rigkit::json::Value* ents = doc.find("entities");
	if (!ents) {
		return nullptr;
	}
	for (rigkit::json::Value& ent : ents->a) {
		rigkit::json::Value* comps = ent.find("components");
		if (!comps) {
			continue;
		}
		if (rigkit::json::Value* component = comps->find(schema)) {
			return component;
		}
	}
	return nullptr;
}

} // namespace

#define CHECK(cond) check((cond), #cond, __LINE__)

int main() {
	using namespace rigkit::espapp;
	std::string err;
	rigkit::json::Value value;
	CHECK(rigkit::json::parse("{\"a\":1,\"b\":\"x\"}", value, err));
	CHECK(value.find("a") && value.find("a")->n == 1);
	CHECK(value.find("b") && value.find("b")->s == "x");
	CHECK(value.dump() == "{\"a\":1,\"b\":\"x\"}");
	CHECK(rigkit::json::parse("\"a\\\"b\"", value, err));
	CHECK(value.s == "a\"b");

	App app;
	Setup(app);
	CHECK(app.document.find("example-esp32") != std::string::npos);
	CHECK(std::strcmp(app.espnow.peer, "ff:ff:ff:ff:ff:ff") == 0);
	CHECK(app.espnow.enabled);
	CHECK(app.radio.mode == RadioMode::Ap);
	const std::string prefs = prefsJson(app);
	CHECK(prefs.find("host.radio") != std::string::npos);
	CHECK(prefs.find("RigKit") != std::string::npos);
	CHECK(prefs.find("sta-secret-value") == std::string::npos);

	Update(app, 0.016f);
	const HalView* view = halView();
	CHECK(view != nullptr);
	CHECK(view->mode == RadioMode::Ap);
	CHECK(view->applyCount >= 1);
	Draw(app);
	CHECK(near(view->r, 0.2f));
	CHECK(near(view->g, 0.55f));
	CHECK(near(view->b, 1.f));

	CHECK(postPass(app, "{\"pass\":\"sta-secret-value\"}", err));
	CHECK(std::strcmp(app.radio.staPass, "sta-secret-value") == 0);
	CHECK(prefsJson(app).find("sta-secret-value") == std::string::npos);
	CHECK(stateJson(app).find("sta-secret-value") == std::string::npos);

	CHECK(postAction(app, "{\"actionId\":\"espnow.send\"}", err));
	Update(app, 0.016f);
	CHECK(std::strcmp(app.espnowCache.lastResult, "esp-now is not in this build") == 0);

	CHECK(patchPrefs(app, "{\"sections\":{\"host.radio\":{\"mode\":\"off\"}}}", err));
	Update(app, 0.016f);
	CHECK(view->mode == RadioMode::Off);
	CHECK(postAction(app, "{\"actionId\":\"espnow.send\"}", err));
	Update(app, 0.016f);
	CHECK(std::strcmp(app.espnowCache.lastResult, "radio is off") == 0);

	halStubPushLine("radio ap");
	Update(app, 0.016f);
	CHECK(view->mode == RadioMode::Ap);

	CHECK(!patchState(app, "{\"rig\":\"1.3.0\",\"entities\":[]}", err));
	CHECK(std::strcmp(app.espnow.peer, "ff:ff:ff:ff:ff:ff") == 0);
	CHECK(!postAction(app, "{\"actionId\":\"nope\"}", err));
	CHECK(!patchPrefs(app, "{\"sections\":{\"host.radio\":{\"mode\":\"nope\"}}}", err));
	CHECK(app.radio.mode == RadioMode::Ap);

	rigkit::json::Value doc;
	CHECK(rigkit::json::parse(app.document.c_str(), doc, err));
	if (rigkit::json::Value* solid = componentOf(doc, "rig.paint.solid")) {
		rigkit::json::Value rgba = rigkit::json::Value::arr();
		rgba.a.push_back(rigkit::json::Value::number(1));
		rgba.a.push_back(rigkit::json::Value::number(0));
		rgba.a.push_back(rigkit::json::Value::number(0));
		rgba.a.push_back(rigkit::json::Value::number(1));
		solid->set("rgba", std::move(rgba));
	}
	if (rigkit::json::Value* now = componentOf(doc, "rig.io.espnow")) {
		now->set("peer", rigkit::json::Value::str("01:02:03:04:05:06"));
		now->set("enabled", rigkit::json::Value::boolean(false));
	}
	CHECK(patchState(app, doc.dump(), err));
	CHECK(std::strcmp(app.espnow.peer, "01:02:03:04:05:06") == 0);
	CHECK(!app.espnow.enabled);
	Draw(app);
	CHECK(near(view->r, 1.f));
	CHECK(near(view->g, 0.f));
	CHECK(postAction(app, "{\"actionId\":\"espnow.send\"}", err));
	Update(app, 0.016f);
	CHECK(std::strcmp(app.espnowCache.lastResult, "esp-now is off") == 0);

	rigkit::json::Value state;
	CHECK(rigkit::json::parse(stateJson(app).c_str(), state, err));
	CHECK(state.find("document") != nullptr);
	CHECK(state.find("cache") != nullptr);

	App again;
	Setup(again);
	CHECK(std::strcmp(again.espnow.peer, "01:02:03:04:05:06") == 0);
	CHECK(!again.espnow.enabled);
	CHECK(again.radio.mode == RadioMode::Ap);
	CHECK(std::strcmp(again.radio.staPass, "sta-secret-value") == 0);
	CHECK(prefsJson(again).find("sta-secret-value") == std::string::npos);
	CHECK(near(again.lamp.r, 1.f));

	if (gFails) {
		std::fprintf(stderr, "%d checks failed\n", gFails);
		return 1;
	}
	std::printf("example-esp32 self-test ok\n");
	return 0;
}
