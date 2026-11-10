// 02 - MIXING TWO COLOURS
//
// WHAT YOU WILL SEE
// Red on the left slowly turning into blue on the right.
//
// TRY THIS
// 1. Swap the two colours. Now change one of them completely.
// 2. Change uv.x to uv.y in the mix(...) line. Which way does the
//    picture turn?
// 3. Change uv.x to uv.x * 3.0. The change now finishes a third of the
//    way across. What does uv.x - 0.5 do?
// 4. Mix between three colours: mix the result with a third colour
//    using a second mix(...) line.
//
// THE ONE BIG IDEA
// mix(a, b, t) blends between two things. t = 0.0 gives all a,
// t = 1.0 gives all b, t = 0.5 gives half and half. Feeding it a
// position makes a gradient: a picture that changes across the screen.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	// Divide the pixel position by the screen size. Now uv.x is 0.0 at
	// the left edge and 1.0 at the right edge, wherever the pixel is.
	// uv.y is 0.0 at the BOTTOM and 1.0 at the top (y grows upwards).
	vec2 uv = fragCoord / iResolution.xy;

	vec3 leftColour = vec3(1.0, 0.2, 0.2);  // red
	vec3 rightColour = vec3(0.2, 0.4, 1.0); // blue

	// uv.x is small on the left and big on the right, so the blend
	// slides from one colour to the other as we cross the screen.
	vec3 colour = mix(leftColour, rightColour, uv.x);

	fragColor = vec4(colour, 1.0);
}
