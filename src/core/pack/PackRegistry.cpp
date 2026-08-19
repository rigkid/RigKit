#include "PackRegistry.h"
#include "IPack.h"

rigkit::PackRegistry& rigkit::PackRegistry::instance() {
	static PackRegistry inst;
	return inst;
}

void rigkit::PackRegistry::addFactory(const std::string& name, Factory f) {
	m_factories[name] = std::move(f);
}

std::shared_ptr<rigkit::IPack> rigkit::PackRegistry::create(const std::string& name) const {
	auto it = m_factories.find(name);
	return it == m_factories.end() ? nullptr : it->second();
}
