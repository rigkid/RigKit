// 18 - JULIA FRACTAL (BOSS LEVEL)
//
// WHAT YOU WILL SEE
// A swirling fractal island that slowly morphs. Hold the mouse down
// over the picture and drag: you are steering the fractal's seed, and
// a tiny move can turn spirals into snowflakes. This is the hardest
// file here - reading it slowly, twice, is normal. Grown-ups do too.
//
// TRY THIS
// 1. Hold the mouse and explore. Gentle moves! Fractals are dramatic.
// 2. Zoom: z = p * 1.4. Change 1.4 to 0.8 (zoom in on the wiggles) or
//    2.2. Fractals look similar at every zoom - that is the point.
// 3. Detail: the loop runs 48 times. Try 16 (fast, blobby) and 100
//    (fine lace - check the FPS counter; this one works hard).
// 4. Colours: the cos palette numbers vec3(0.0, 2.0, 4.0). Change them.
// 5. Still picture: take the iTime wave out of the seed line:
//    vec2 seed = vec2(-0.7, 0.27); - a famous Julia set, frozen.
//
// THE ONE BIG IDEA
// Feed a number back into the same sum, over and over: z becomes
// z-squared-plus-seed, 48 times per pixel. Some starting points stay
// small; others escape to huge. Counting HOW LONG each point survives
// before escaping paints the fractal - detail forever, from one sum.

// One step of the fractal sum. "Squaring" a vec2 (a complex number)
// is (x*x - y*y, 2*x*y) - that is the rule, take it on trust or ask
// a search engine about complex numbers when you are ready.
vec2 squareIt(vec2 z) {
	return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	vec2 z = p * 1.4; // where this pixel starts

	// The seed slowly wanders when nobody is steering...
	vec2 seed = vec2(-0.55 + 0.25 * sin(iTime * 0.11), 0.25 * cos(iTime * 0.13));
	// ...and the mouse takes over while the button is held.
	if (iMouse.z > 0.0) {
		seed = (iMouse.xy - 0.5 * iResolution.xy) / iResolution.y * 0.9;
	}

	// Escape counting: survive all 48 rounds and you are "inside".
	float survived = 48.0;
	for (int i = 0; i < 48; i++) {
		z = squareIt(z) + seed;
		if (dot(z, z) > 4.0) { // escaped: bigger than 2, it never returns
			survived = float(i);
			break;
		}
	}

	// Inside is near-black. Outside, colour by how long it survived,
	// with example 03's rainbow wave.
	vec3 colour = vec3(0.02);
	if (survived < 48.0) {
		colour = 0.5 + 0.5 * cos(survived * 0.25 + vec3(0.0, 2.0, 4.0));
	}

	fragColor = vec4(colour, 1.0);
}
