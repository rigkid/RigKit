#pragma once

#include <string>

#include "ecs/MEcs.h"

namespace stage {

/**
 * @brief Import a mesh file as entity `prop` and select it.
 * @details **rigAssimp** (glTF and the other formats that build enables).
 * Diffuse, when Assimp reports one, lands on `CMesh::faceColors`. UVs land on
 * `CMesh::texcoords`. The present ignores those UVs until a scene `CIndexedAtlas`
 * exists. @p err receives the Assimp error when the load fails.
 */
bool addProp(rigkit::MEcs& ecs, const std::string& path, std::string* err);

} // namespace stage
