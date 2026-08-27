#include "core/util/Progress.h"

#include <algorithm>

namespace rigkit {

void Progress::setUseStatusBar(bool useStatusBar) {
	m_useStatusBar = useStatusBar;
}

bool Progress::useStatusBar() const {
	return m_useStatusBar;
}

void Progress::setAutoHideDelay(float seconds) {
	m_autoHideDelay = std::max(0.f, seconds);
}

float Progress::autoHideDelay() const {
	return m_autoHideDelay;
}

void Progress::setCancelable(bool cancelable) {
	m_cancelable = cancelable;
}

bool Progress::cancelable() const {
	return m_cancelable;
}

void Progress::begin(std::string title, int totalSteps) {
	m_cancelRequested.store(false, std::memory_order_relaxed);
	std::lock_guard lock(m_mutex);
	m_title = std::move(title);
	m_label.clear();
	m_progress = 0.f;
	m_totalSteps = std::max(totalSteps, 1);
	m_currentStep = 0;
	m_active = true;
	m_finished = false;
	m_finishRemain = -1.f;
}

void Progress::begin(std::string title) {
	m_cancelRequested.store(false, std::memory_order_relaxed);
	std::lock_guard lock(m_mutex);
	m_title = std::move(title);
	m_label.clear();
	m_progress = 0.f;
	m_totalSteps = 0;
	m_currentStep = 0;
	m_active = true;
	m_finished = false;
	m_finishRemain = -1.f;
}

void Progress::tick(std::string label) {
	std::lock_guard lock(m_mutex);
	if (!m_active) {
		return;
	}
	m_label = std::move(label);
	if (m_totalSteps > 0) {
		m_currentStep = std::min(m_currentStep + 1, m_totalSteps);
		m_progress = static_cast<float>(m_currentStep) / static_cast<float>(m_totalSteps);
	}
}

void Progress::tick(std::string label, float prog) {
	std::lock_guard lock(m_mutex);
	if (!m_active) {
		return;
	}
	m_label = std::move(label);
	m_progress = (prog < 0.f) ? -1.f : std::min(prog, 1.f);
}

void Progress::tickIndeterminate(std::string label) {
	tick(std::move(label), -1.f);
}

void Progress::finish(std::string doneLabel) {
	std::lock_guard lock(m_mutex);
	m_progress = 1.f;
	m_label = std::move(doneLabel);
	m_active = false;
	if (m_autoHideDelay <= 0.f) {
		m_finished = false;
		m_finishRemain = -1.f;
		m_title.clear();
		m_label.clear();
		return;
	}
	m_finished = true;
	m_finishRemain = m_autoHideDelay;
}

void Progress::hide() {
	{
		std::lock_guard lock(m_mutex);
		m_active = false;
		m_finished = false;
		m_finishRemain = -1.f;
		m_title.clear();
		m_label.clear();
		m_progress = 0.f;
	}
	m_cancelRequested.store(false, std::memory_order_relaxed);
}

void Progress::requestCancel() {
	m_cancelRequested.store(true, std::memory_order_relaxed);
	std::lock_guard lock(m_mutex);
	if (m_active) {
		m_label = "Cancelling...";
	}
}

bool Progress::cancelRequested() const {
	return m_cancelRequested.load(std::memory_order_relaxed);
}

bool Progress::isActive() const {
	std::lock_guard lock(m_mutex);
	return m_active;
}

float Progress::progress() const {
	std::lock_guard lock(m_mutex);
	return m_progress;
}

std::string Progress::label() const {
	std::lock_guard lock(m_mutex);
	return m_label;
}

std::string Progress::title() const {
	std::lock_guard lock(m_mutex);
	return m_title;
}

Progress::Snapshot Progress::snapshot() const {
	std::lock_guard lock(m_mutex);
	Snapshot s;
	s.title = m_title;
	s.label = m_label;
	s.progress = m_progress;
	s.active = m_active;
	s.finished = m_finished;
	s.visible = m_active || m_finished;
	s.cancelable = m_cancelable;
	return s;
}

void Progress::tickFrame(float dt) {
	std::lock_guard lock(m_mutex);
	if (!m_finished || m_finishRemain < 0.f) {
		return;
	}
	m_finishRemain -= dt;
	if (m_finishRemain <= 0.f) {
		m_finished = false;
		m_finishRemain = -1.f;
		m_title.clear();
		m_label.clear();
		m_progress = 0.f;
	}
}

} // namespace rigkit
