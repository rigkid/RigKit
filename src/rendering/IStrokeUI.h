#pragma once

#include <string>
#include <vector>

namespace rigkit {

// Generic stroke parameters used by UI back-ends (Blend2D, OpenGL, etc.)
struct StrokeState {
	float width = 1.0f;
	int cap = 0;  // backend-specific enum value
	int join = 0; // backend-specific enum value
	float miterLimit = 4.0f;
	std::vector<double> dashArray;
	float dashOffset = 0.0f;
	int transformOrder = 0; // backend-specific enum value
};

// Minimal interface a stroke-editing widget must implement.
class IStrokeUI {
  public:
	virtual ~IStrokeUI() = default;

	// Draw the UI; implementor should modify 'state' directly.
	virtual void show(const char* label, StrokeState& state) = 0;
};

} // namespace rigkit
