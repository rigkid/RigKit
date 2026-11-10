// 12 - SUNSET (PUTTING PIECES TOGETHER)
//
// WHAT YOU WILL SEE
// A little scene: gradient sky, a glowing sun sinking into a sea that
// sparkles. Nothing here is new - it is examples 02, 05 and 09 stacked
// in layers, back to front. That is how bigger pictures get built.
//
// TRY THIS
// 1. Sink the sun: sunCentre y from 0.05 to -0.1. Golden hour.
// 2. Change the sky colours. Stormy grey? Alien green?
// 3. Move the sea level: the 0.3 line (uv.y is 0 at the bottom).
// 4. Bigger waves on the sea: 60.0 in the sparkle sin(...), or faster
//    iTime * 3.0.
// 5. Add a second circle as a moon, high in the sky, pale and small.
//    (Example 05 has the circle recipe.)
// 6. Birds: too hard to draw? A tiny dark "v" is two short lines -
//    skip this until you feel brave. Scenes grow one layer at a time.
//
// THE ONE BIG IDEA
// Layers. Paint the far thing first, mix the next thing over it, then
// the next. Order matters: sea after sun covers the sun's bottom -
// which is exactly why the sun looks like it sinks INTO the water.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// Layer 1: sky. Warm at the horizon, purple up high.
	vec3 colour = mix(vec3(1.0, 0.55, 0.25), vec3(0.2, 0.1, 0.45), uv.y);

	// Layer 2: the sun - a soft circle (example 05) plus a wide glow.
	vec2 sunCentre = vec2(0.0, 0.05);
	float sunDist = length(p - sunCentre);
	float sun = 1.0 - smoothstep(0.14, 0.16, sunDist);
	float glow = 1.0 - smoothstep(0.0, 0.55, sunDist);
	colour += vec3(1.0, 0.5, 0.15) * glow * 0.5;		 // add light
	colour = mix(colour, vec3(1.0, 0.85, 0.5), sun); // then the disc

	// Layer 3: sea over the bottom of everything, with moving sparkle.
	float sea = 1.0 - smoothstep(0.29, 0.31, uv.y);
	float sparkle = 0.5 + 0.5 * sin(uv.x * 60.0 + iTime * 2.0 + sin(uv.y * 40.0) * 2.0);
	vec3 seaColour = vec3(0.05, 0.15, 0.35) + vec3(1.0, 0.6, 0.3) * sparkle * 0.12;
	colour = mix(colour, seaColour, sea);

	fragColor = vec4(colour, 1.0);
}
