#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "rendering/U_rendering.h"

class RendererRegistry {
  public:
	using Factory = std::function<std::shared_ptr<IRenderer>()>;

	static RendererRegistry& instance() {
		static RendererRegistry registry;
		return registry;
	}

	void registerFactory(RendererType type, Factory factory) {
		factories[type] = std::move(factory);
	}

	std::shared_ptr<IRenderer> create(RendererType type) {
		auto it = factories.find(type);
		if (it != factories.end()) {
			return it->second();
		}
		return nullptr;
	}

	bool has(RendererType type) const { return factories.find(type) != factories.end(); }

	std::vector<RendererType> registeredTypes() const {
		std::vector<RendererType> out;
		out.reserve(factories.size());
		for (const auto& [type, _] : factories) {
			out.push_back(type);
		}
		return out;
	}

  private:
	std::unordered_map<RendererType, Factory> factories;
};
