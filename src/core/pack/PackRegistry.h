#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace rigkit {
class IPack;

class PackRegistry {
  public:
	using Factory = std::function<std::shared_ptr<IPack>()>;
	static PackRegistry& instance();
	void addFactory(const std::string& name, Factory f);
	std::shared_ptr<IPack> create(const std::string& name) const;

  private:
	std::unordered_map<std::string, Factory> m_factories;
};

// Helper macro for packs to self-register a factory at static-init time
// DEPRECATED: Use MPack::registerPack() instead
#define RIGKIT_REGISTER_PACK(PACK_CLASS)                                                           \
	namespace {                                                                                    \
	struct PACK_CLASS##Registrar {                                                                 \
		PACK_CLASS##Registrar() {                                                                  \
			rigkit::PackRegistry::instance().addFactory(                                           \
				#PACK_CLASS, []() { return std::make_shared<PACK_CLASS>(); });                     \
		}                                                                                          \
	};                                                                                             \
	static PACK_CLASS##Registrar PACK_CLASS##_auto_reg;                                            \
	}
} // namespace rigkit