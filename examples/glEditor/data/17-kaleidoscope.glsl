// 17 - KALEIDOSCOPE
//
// WHAT YOU WILL SEE
// A spinning mirrored pattern, like looking into a kaleidoscope tube.
// It takes the plasma (example 10) and folds space before drawing it.
//
// TRY THIS
// 1. Mirrors: slices 8.0 to 6.0 or 12.0. Even numbers behave best.
// 2. Spin speed: iTime * 0.1 in the angle line - and flip its sign.
// 3. Zoom: p * 1.5 to p * 0.7. What does zoom do inside a kaleidoscope?
// 4. Different picture inside: swap the plasma lines for example 09's
//    rings (colour by length(folded)) or example 04's stripes.
// 5. Add a slow zoom pulse: multiply folded by
//    1.0 + 0.2 * sin(iTime * 0.5).
//
// THE ONE BIG IDEA
// Fold the world, then draw once. Cutting a full circle into slices
// and mirroring every slice (abs does the mirroring) means anything
// drawn in one slice appears in all of them - symmetry for free.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// Polar coordinates again (example 16): which way, how far.
	float angle = atan(p.y, p.x) + iTime * 0.1; // slowly spinning
	float radius = length(p);

	// Fold: chop the circle into slices, mirror each slice in half.
	float slice = 6.2831 / 8.0;			// one slice of the pie (8 slices)
	float foldedAngle = abs(mod(angle, slice) - slice * 0.5);

	// Back to (x, y) - but now every direction is the same slice.
	vec2 folded = vec2(cos(foldedAngle), sin(foldedAngle)) * radius;

	// Draw plasma (example 10) into the folded space.
	float v = sin(folded.x * 12.0 + iTime) + sin(folded.y * 12.0 - iTime * 0.7);
	vec3 colour = 0.5 + 0.5 * cos(v * 1.5 + vec3(0.0, 2.0, 4.0));

	// Darken the rim so it feels like looking down a tube.
	colour *= 1.0 - smoothstep(0.55, 1.0, radius);

	fragColor = vec4(colour, 1.0);
}
