// 05 - A SOFT CIRCLE
//
// WHAT YOU WILL SEE
// One fuzzy-edged circle in the middle of a dark screen. Almost every
// shader picture is built like this: measure how far each pixel is
// from a shape, then colour by that distance.
//
// TRY THIS
// 1. Change 0.3 (the size) to 0.1, then 0.45. What happens at 0.6?
// 2. The edge lives between 0.26 and 0.3. Make the gap bigger, like
//    0.1 and 0.3 - a much softer, smokier edge.
// 3. Move the circle: change the centre line to
//    vec2 centre = vec2(0.25, -0.2);  (x right, y up, 0 is the middle)
// 4. Two circles: copy the circle lines, use a second centre and a
//    second colour, and mix it over the first result.
// 5. Change the circle colour every frame with example 03's rainbow
//    trick: vec3(0.5 + 0.5 * cos(iTime + vec3(0.0, 2.0, 4.0)))
//
// THE ONE BIG IDEA
// length(p) is the distance from the middle - Pythagoras from maths
// class, done for every pixel at once. smoothstep(a, b, x) blends
// smoothly from 0 to 1 as x crosses from a to b, which makes edges
// soft instead of jagged.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	// Recentring trick: p is (0, 0) in the middle of the screen,
	// about -0.5 at the bottom and +0.5 at the top. Dividing by
	// iResolution.y (not .x) keeps circles round, not squished.
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	vec2 centre = vec2(0.0, 0.0);
	float dist = length(p - centre); // how far this pixel is from centre

	// Inside 0.26 we are fully in the circle; outside 0.3 fully out.
	// 1.0 - smoothstep(...) flips it so "in" means 1.
	float circle = 1.0 - smoothstep(0.26, 0.3, dist);

	vec3 background = vec3(0.05, 0.05, 0.15); // deep blue
	vec3 circleColour = vec3(1.0, 0.6, 0.15); // orange

	vec3 colour = mix(background, circleColour, circle);

	fragColor = vec4(colour, 1.0);
}
