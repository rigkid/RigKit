#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace rigkit {

/**
 * @brief Thread-safe progress chrome state - no UI toolkit types.
 * @details Apps/packs call begin / tick / finish via `IMui::progress()`.
 *          **rigImGui** presents status-bar or floating fulfillment.
 * @see docs/contract/ui.md
 */
class Progress {
  public:
	struct Snapshot {
		std::string title;
		std::string label;
		float progress = 0.f; ///< 0..1, or <0 for indeterminate marquee
		bool active = false;
		bool finished = false;
		bool visible = false; ///< active or finished (still showing)
		bool cancelable = false;
	};

	void setUseStatusBar(bool useStatusBar);
	bool useStatusBar() const;

	/// Seconds to keep the panel after finish(); 0 = hide immediately.
	void setAutoHideDelay(float seconds);
	float autoHideDelay() const;

	void setCancelable(bool cancelable);
	bool cancelable() const;

	/// Step mode: each tick(label) advances by 1/totalSteps.
	void begin(std::string title, int totalSteps);
	/// Absolute / indeterminate mode: supply fraction via tick(label, progress).
	void begin(std::string title);

	void tick(std::string label);
	void tick(std::string label, float progress);
	void tickIndeterminate(std::string label);

	void finish(std::string doneLabel = "Done");
	void hide();

	void requestCancel();
	bool cancelRequested() const;

	bool isActive() const;
	float progress() const;
	std::string label() const;
	std::string title() const;

	Snapshot snapshot() const;

	/// Drive auto-hide countdown (call once per frame from UI fulfillments).
	void tickFrame(float dt);

  private:
	mutable std::mutex m_mutex;

	std::string m_title;
	std::string m_label;
	float m_progress = 0.f;
	int m_totalSteps = 0;
	int m_currentStep = 0;
	bool m_active = false;
	bool m_finished = false;
	float m_finishRemain = -1.f;
	std::atomic<bool> m_cancelRequested{false};

	bool m_useStatusBar = true;
	bool m_cancelable = false;
	float m_autoHideDelay = 2.0f;
};

} // namespace rigkit
