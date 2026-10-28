// 09 - TARGET RINGS
//
// WHAT YOU WILL SEE
// Rainbow rings spreading out from the middle, like ripples in a pond.
//
// TRY THIS
// 1. Change 25.0 (rings squeezed together) and the speed iTime * 2.0.
// 2. Change - iTime to + iTime. The ripples run the other way.
// 3. Move the middle: centre = vec2(0.3, 0.2). Then make the middle
//    follow the mouse (example 07 shows how to convert iMouse).
// 4. Sharpen the rings: colour = vec3(step(0.5, wave)); Flat bands!
//    Then soften just a bit with smoothstep(0.4, 0.6, wave).
// 5. One colour only: multiply the rainbow by vec3(1.0, 0.5, 0.0).
//
// THE ONE BIG IDEA
// Rings are just "colour depends on distance". Distance makes circles
// for free (every point at distance 0.3 from the centre IS a circle),
// and a sin wave over distance paints ring after ring.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	vec2 centre = vec2(0.0, 0.0);
	float dist = length(p - centre);

	// A wave that travels outwards: distance does the ringing,
	// subtracting iTime slides the wave away from the centre.
	float wave = 0.5 + 0.5 * sin(dist * 25.0 - iTime * 2.0);

	// Example 03's rainbow trick, driven by the wave instead of position.
	vec3 colour = 0.5 + 0.5 * cos(wave * 6.2831 + vec3(0.0, 2.0, 4.0));

	// Fade to dark at the edges so the target floats on the screen.
	colour *= 1.0 - smoothstep(0.5, 0.9, dist);

	fragColor = vec4(colour, 1.0);
}
