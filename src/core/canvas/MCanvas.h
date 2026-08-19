#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/IManager.h"
#include "core/ISettings.h"
#include "Canvas.h"

namespace rigkit {

class RigKitEngine;

/**
 * @brief Owns named Canvas instances and the active-canvas selection.
 * @see Canvas
 */
class MCanvas : public IManager, public ISettings {
  public:
	MCanvas(RigKitEngine* engine);
	~MCanvas();
	void init() override {}
	void shutdown() override {}

	std::shared_ptr<Canvas> createCanvas(const CanvasSettings& settings,
										 const std::string& name = "");
	void removeCanvas(size_t index);
	void removeCanvas(const std::string& name);
	void renameCanvas(size_t index, const std::string& newName);
	void renameCanvas(const std::string& oldName, const std::string& newName);

	// Active canvas management
	void setActiveCanvas(size_t index);
	void setActiveCanvas(const std::string& name);
	std::shared_ptr<Canvas> getActiveCanvas() const;
	size_t getActiveCanvasIndex() const;
	std::string getActiveCanvasName() const;

	// Canvas queries
	std::vector<std::shared_ptr<Canvas>> getAllCanvases() const;
	std::shared_ptr<Canvas> getCanvas(size_t index) const;
	std::shared_ptr<Canvas> getCanvas(const std::string& name) const;
	size_t getCanvasCount() const;
	size_t findCanvasIndex(const std::string& name) const;
	bool hasCanvas(const std::string& name) const;

	// Canvas information
	std::vector<std::string> getAllCanvasNames() const;
	std::vector<std::pair<size_t, std::string>> getAllCanvasInfo() const;

	// Event callbacks
	void setOnCanvasCreated(std::function<void(size_t, const std::string&)> callback) {
		m_onCanvasCreated = callback;
	}
	void setOnCanvasRemoved(std::function<void(size_t, const std::string&)> callback) {
		m_onCanvasRemoved = callback;
	}
	void setOnActiveCanvasChanged(std::function<void(size_t, const std::string&)> callback) {
		m_onActiveCanvasChanged = callback;
	}
	void setOnCanvasRenamed(
		std::function<void(size_t, const std::string&, const std::string&)> callback) {
		m_onCanvasRenamed = callback;
	}

	// Utility
	void clear();
	bool isEmpty() const { return m_canvases.empty(); }

	// ISettings interface
	json getSettings() const override;
	void setSettings(const json& settings) override;

  private:
	std::vector<std::shared_ptr<Canvas>> m_canvases;
	size_t m_activeCanvasIndex;
	RigKitEngine* m_engine;

	// Event callbacks
	std::function<void(size_t, const std::string&)> m_onCanvasCreated;
	std::function<void(size_t, const std::string&)> m_onCanvasRemoved;
	std::function<void(size_t, const std::string&)> m_onActiveCanvasChanged;
	std::function<void(size_t, const std::string&, const std::string&)> m_onCanvasRenamed;

	// Helper methods
	void validateIndex(size_t index) const;
	void validateName(const std::string& name) const;
	std::string generateDefaultName() const;
};

} // namespace rigkit
