// 06 - BOUNCING BALL
//
// WHAT YOU WILL SEE
// A ball that bounces up and down on a floor, all by itself.
//
// TRY THIS
// 1. Change the bounce speed: iTime * 2.0 in the height line.
// 2. Change 0.35 (how high) - and -0.3 (where the floor is). Can you
//    make the ball roll through the floor? What went wrong?
// 3. Give the ball sideways movement too:
//    float ballX = sin(iTime * 0.9) * 0.5;
//    and use vec2(ballX, ballY) as the centre.
// 4. Change the ball's size (0.12) as it bounces, so it squashes a bit:
//    size = 0.12 + 0.02 * sin(iTime * 4.0);
// 5. Make a second, smaller ball with a different bounce speed.
//
// THE ONE BIG IDEA
// Animation means "position depends on iTime". sin(...) waves up and
// down, but a real bounce stays above the floor: abs(x) flips negative
// numbers positive, turning a smooth wave into a bouncy V shape.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
	vec2 uv = fragCoord / iResolution.xy;

	// The floor sits at y = -0.3. The ball's height bounces above it:
	// abs(sin(...)) is 0, up to 1, back to 0 - bounce, bounce, bounce.
	float floorY = -0.3;
	float ballY = floorY + 0.1 + abs(sin(iTime * 2.0)) * 0.35;
	vec2 ballCentre = vec2(0.0, ballY);

	float dist = length(p - ballCentre);
	float ball = 1.0 - smoothstep(0.11, 0.12, dist);

	// Sky gradient (example 02's trick), then floor, then ball on top.
	vec3 colour = mix(vec3(0.1, 0.2, 0.5), vec3(0.4, 0.7, 1.0), uv.y);
	if (p.y < floorY) {
		colour = vec3(0.15, 0.4, 0.2); // grass-green floor
	}
	colour = mix(colour, vec3(1.0, 0.3, 0.25), ball); // red ball

	fragColor = vec4(colour, 1.0);
}
