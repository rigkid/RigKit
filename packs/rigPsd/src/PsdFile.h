#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rigkit {
namespace psd {

/**
 * @brief One raster layer as stored by this pack (not Photoshop's full tagged-block graph).
 * @details Bounds are document pixels. `rgba` is tightly packed 8-bit unassociated RGB+A
 * at `w` x `h`. Unknown Photoshop blocks are not modelled.
 */
struct PsdLayer {
	std::string name = "Layer";
	bool visible = true;
	float opacity = 1.f;
	int blendMode = 0; ///< PixelBlendMode integer when mapped by the pixel pack
	int left = 0;
	int top = 0;
	int w = 0;
	int h = 0;
	std::vector<uint8_t> rgba;
};

/**
 * @brief Format-level PSD document this pack can read and write.
 * @details 8-bit RGB only. No text, smart objects, or adjustment layers.
 */
struct PsdFile {
	int width = 0;
	int height = 0;
	std::vector<PsdLayer> layers;
};

/** @brief Read a layered 8-bit RGB PSD into @p out. */
bool readFile(const std::string& path, PsdFile& out);

/** @brief Write modelled layers as a layered 8-bit RGB PSD. */
bool writeFile(const std::string& path, const PsdFile& file);

} // namespace psd
} // namespace rigkit
