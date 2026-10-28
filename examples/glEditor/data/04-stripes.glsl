// 04 - STRIPES
//
// WHAT YOU WILL SEE
// Black and white bars sliding sideways.
//
// TRY THIS
// 1. Change 10.0 to 3.0, then to 40.0. What does that number count?
// 2. Change 0.5 in the step(...) to 0.2 or 0.8. Fat stripe or thin?
// 3. Change uv.x to uv.y. Change it to uv.x + uv.y for diagonal stripes.
// 4. Take out "+ iTime". Still or moving - which do you like?
// 5. Replace the black and white vec3 colours with two wild colours.
//
// THE ONE BIG IDEA
// fract(x) throws away the whole-number part and keeps the fraction:
// fract(2.75) is 0.75. So fract(uv.x * 10.0) counts 0 to 1, ten times
// across the screen - a sawtooth ramp. step(edge, x) is a tiny judge:
// 0.0 when x is below the edge, 1.0 when above. Ramp plus judge = stripes.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;

	// Count 0 to 1 over and over as we move right. Adding iTime * 0.5
	// pushes the counting along, so the stripes slide.
	float ramp = fract(uv.x * 10.0 + iTime * 0.5);

	// First half of each count is 0.0 (black), second half is 1.0 (white).
	float stripe = step(0.5, ramp);

	vec3 colour = mix(vec3(0.0), vec3(1.0), stripe);

	fragColor = vec4(colour, 1.0);
}
