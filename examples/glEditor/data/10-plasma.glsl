// 10 - PLASMA
//
// WHAT YOU WILL SEE
// Slow, blobby, lava-lamp colours. Old demo-scene favourite: add a
// few sine waves together and colour the total.
//
// TRY THIS
// 1. Comment out one of the v += lines (put // in front). What shape
//    does each wave add? Put them back one at a time.
// 2. Change 10.0 in the first line to 3.0 or 25.0 - bigger or smaller
//    blobs.
// 3. Change the speeds: t * 1.3 to t * 0.2. Mix a fast and a slow wave.
// 4. Swap the palette colours vec3(0.0, 2.0, 4.0) for
//    vec3(4.0, 2.0, 0.0) - same blobs, different fire.
// 5. Multiply uv by 2.0 at the top: zoom in on the lava.
//
// THE ONE BIG IDEA
// Waves add. One sine makes stripes; four sines pointing different
// ways, at different speeds, interfere with each other and make
// organic blobs no single wave could make. Colour is the last step.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;
	float t = iTime; // a shorter name for the clock

	// Four waves: across, up, diagonal, and out from the middle.
	float v = sin(uv.x * 10.0 + t);
	v += sin(uv.y * 10.0 + t * 1.3);
	v += sin((uv.x + uv.y) * 8.0 + t * 0.7);
	v += sin(length(uv - 0.5) * 14.0 - t);
	v *= 0.25; // average them (they added up to -4..4, now -1..1)

	// Turn the total into colour with example 03's wave trick.
	vec3 colour = 0.5 + 0.5 * cos(3.14159 * v + vec3(0.0, 2.0, 4.0));

	fragColor = vec4(colour, 1.0);
}
