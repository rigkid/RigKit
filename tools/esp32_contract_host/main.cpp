// SUDE + RigKit-esp32-core proof sketch
// Desktop host self-test (no ESP-IDF required). Copy into ESP-IDF/Arduino as app logic.
// See docs/contract/rigkit.md#esp32-subset-profile

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

// ---- SUDE hooks (firmware-shaped) -------------------------------------------

struct AppState;

void Setup(AppState& app);
void Update(AppState& app, float dt);
void Draw(AppState& app); // may be empty on MCU
void Exit(AppState& app);

// ---- RigKit-esp32-core: minimal POD entity table ------------------------------

struct EntityPod {
	uint32_t id = 0;
	char tag[16] = {};
	float x = 0.f;
	float y = 0.f;
	float intensity = 0.f; // LED / modulator
};

struct AppState {
	std::vector<EntityPod> entities;
	bool running = true;
	float time = 0.f;
	int frames = 0;
};

void Setup(AppState& app) {
	app.entities.clear();
	EntityPod led{};
	led.id = 1;
	std::snprintf(led.tag, sizeof(led.tag), "led0");
	led.intensity = 0.f;
	app.entities.push_back(led);

	EntityPod sensor{};
	sensor.id = 2;
	std::snprintf(sensor.tag, sizeof(sensor.tag), "sensor0");
	app.entities.push_back(sensor);

	std::puts("[esp32_contract_host] Setup — SUDE + RigKit-esp32-core");
}

void Update(AppState& app, float dt) {
	app.time += dt;
	++app.frames;
	for (auto& e : app.entities) {
		if (std::strcmp(e.tag, "led0") == 0) {
			e.intensity = 0.5f + 0.5f * std::sin(app.time * 2.f);
		}
		if (std::strcmp(e.tag, "sensor0") == 0) {
			e.x = app.time; // stand-in for a sensor reading
		}
	}
	if (app.frames >= 120) {
		app.running = false; // self-test exits after ~2s at 60Hz
	}
}

void Draw(AppState&) {
	// Body may be empty; host still calls Draw every tick. LED/GPIO present goes here.
}

void Exit(AppState& app) {
	std::printf("[esp32_contract_host] Exit — frames=%d entities=%zu led.intensity=%.2f\n",
				app.frames, app.entities.size(),
				app.entities.empty() ? 0.f : app.entities[0].intensity);
}

#ifndef RIGKIT_MCU
// Host self-test (desktop). On firmware, define RIGKIT_MCU and call Setup/Update/Draw/Exit
// from the board loop (ESP-IDF, Arduino, …).
int main() {
	AppState app;
	Setup(app);
	using clock = std::chrono::steady_clock;
	auto last = clock::now();
	while (app.running) {
		auto now = clock::now();
		float dt = std::chrono::duration<float>(now - last).count();
		last = now;
		Update(app, dt);
		Draw(app);
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
	Exit(app);
	return 0;
}
#endif
