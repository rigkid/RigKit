// 13 - WAVY PICTURE (BEND THE COORDINATES)
//
// WHAT YOU WILL SEE
// A checkerboard seen through wavy water. The board code did not
// change at all - we bent the coordinates BEFORE drawing the board.
//
// TRY THIS
// 1. Change 0.05 (how bendy) to 0.0 (flat), 0.15 (very wobbly).
// 2. Change 8.0 (wave count up the screen) and iTime * 1.5 (speed).
// 3. Bend x as well: add a line bending uv2.y using uv2.x.
// 4. Warp a different picture: swap the checkerboard half for the
//    stripes from example 04. Bending works on ANY picture, because
//    it happens before the picture is drawn.
// 5. Mouse waves: bend by distance from the mouse instead of by uv.y.
//    (Example 07 shows the mouse trick. This one is a puzzle.)
//
// THE ONE BIG IDEA
// uv is a question: "which part of the pattern am I looking at?"
// Change the question - add a wiggle to uv - and the whole pattern
// wiggles, without the pattern code knowing anything about it.
// Most tricks in shaders are coordinate tricks.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;

	// Bend: slide x sideways by a sine wave that depends on y.
	vec2 uv2 = uv;
	uv2.x += sin(uv.y * 8.0 + iTime * 1.5) * 0.05;

	// Exactly the checkerboard from example 08, fed the bent uv.
	vec2 square = floor(uv2 * 8.0);
	float check = mod(square.x + square.y, 2.0);
	vec3 colour = mix(vec3(0.1, 0.15, 0.4), vec3(0.95, 0.9, 0.7), check);

	fragColor = vec4(colour, 1.0);
}
