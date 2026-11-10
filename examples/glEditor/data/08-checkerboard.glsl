// 08 - CHECKERBOARD
//
// WHAT YOU WILL SEE
// A chess board that slowly zooms in and out.
//
// TRY THIS
// 1. Change 8.0 (squares across) to 4.0 or 16.0.
// 2. Replace the zoom line with float zoom = 8.0; - a board that sits
//    still. Then try zoom = 8.0 + iTime; - it zooms forever. Where
//    does it go wrong? (Hint: squares shrink to nothing, then flip.)
// 3. Move the board: add vec2(iTime * 0.2, 0.0) to uv before scaling.
// 4. Recolour: black/white is classic, but try navy and gold.
// 5. Rotate your view: swap uv.x and uv.y in the grid line.
//
// THE ONE BIG IDEA
// floor(x) rounds DOWN to a whole number: floor(2.75) is 2. So
// floor(uv * 8.0) gives each pixel a square number, like (3, 5).
// Add the two numbers: even sums are one colour, odd the other.
// mod(n, 2.0) answers "what is left over after dividing by 2" -
// the even/odd test.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;

	// Breathe in and out between 6 and 10 squares across.
	float zoom = 8.0 + 2.0 * sin(iTime * 0.5);

	// Which square is this pixel in? Whole numbers only.
	vec2 square = floor(uv * zoom);

	// Even squares one colour, odd squares the other.
	float check = mod(square.x + square.y, 2.0);

	vec3 colour = mix(vec3(0.08), vec3(0.95), check);

	fragColor = vec4(colour, 1.0);
}
