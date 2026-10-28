// 16 - TUNNEL (POLAR COORDINATES)
//
// WHAT YOU WILL SEE
// Flying through a checkered tunnel that never ends. It is a flat
// checkerboard (example 08) wrapped around a circle - no 3D at all.
//
// TRY THIS
// 1. Flight speed: iTime * 1.0 in the depth line. Faster!
// 2. Squares around: angle * 4.0 to angle * 8.0. And rings: * 1.0 in
//    the depth part of the check line to * 2.0.
// 3. Twist the tunnel: depth += angle * 0.3; before the check line.
// 4. Recolour: tunnel walls in two new colours. Then colour by depth
//    with example 03's rainbow instead of a checkerboard.
// 5. Bend the flight path: tunnelCentre = vec2(sin(iTime*0.5)*0.2, 0.0)
//    and use p - tunnelCentre everywhere p is used.
//
// THE ONE BIG IDEA
// Polar coordinates describe a pixel by (angle, distance) instead of
// (x, y). Feed ANGLE to one side of a checkerboard and 1/distance to
// the other, and the flat board becomes a tunnel: far-away pattern is
// squeezed near the centre, and sliding it looks like flying forward.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// The two polar questions: which way? how far?
	float angle = atan(p.y, p.x); // -3.14 to 3.14, around the circle
	float radius = length(p);	  // 0 in the very middle

	// Radius is tiny in the middle, so 1/radius is huge there: pattern
	// near the centre is the "far away" end of the tunnel, squeezed
	// small. Sliding it with iTime makes the pattern rush out at us.
	float depth = 0.35 / max(radius, 0.001) - iTime * 1.0;

	// Checkerboard in (around, depth) space instead of (x, y).
	float check = mod(floor(angle * 4.0) + floor(depth), 2.0);
	vec3 colour = mix(vec3(0.08, 0.05, 0.2), vec3(1.0, 0.6, 0.2), check);

	// Darken the deep middle so the tunnel has an end-of-the-world glow
	// of darkness, and brighten the rim a touch.
	colour *= smoothstep(0.0, 0.35, radius);

	fragColor = vec4(colour, 1.0);
}
