#pragma once

/**
 * @file
 * @brief Register `rig.font.*` codecs on a document serializer registry.
 * @details Call from rigFontComponent::setup when rigProject is loaded.
 */

namespace rigkit {
namespace project {
class ComponentSerializerRegistry;
}

namespace font {

void registerProjectSerializers(project::ComponentSerializerRegistry& registry);

} // namespace font
} // namespace rigkit
