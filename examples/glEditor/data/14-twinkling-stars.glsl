// 14 - TWINKLING STARS
//
// WHAT YOU WILL SEE
// A night sky: every cell of an invisible grid either holds a small
// twinkling star or stays dark.
//
// TRY THIS
// 1. More stars: the cut-off 0.92 in the star line. 0.8 is crowded,
//    0.98 is nearly empty sky.
// 2. Star size: 0.08 in the smoothstep. Bigger numbers, smaller stars
//    here - the star fades from 0.01 to that distance.
// 3. Grid fineness: p * 12.0 (both hash lines and cell line) to
//    p * 24.0 for tiny far stars.
// 4. Shooting star idea: add a bright circle (example 05) whose centre
//    slides across with fract(iTime * 0.1). Two lines of code.
// 5. Give the sky a horizon glow: mix the background by uv.y.
//
// THE ONE BIG IDEA
// hash(...) makes "random" numbers that are NOT random: the same cell
// always gets the same number, every frame, forever. Real randomness
// would boil and flicker. A hash scrambles a position into a number
// that looks random but holds still - shaders use it everywhere.

// Scramble a cell number into a 0-to-1 number that looks random.
float hash(vec2 cell) {
	return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// floor/fract team again (example 11): cell name + place inside it.
	vec2 id = floor(p * 12.0);
	vec2 cell = fract(p * 12.0) - 0.5;

	// This cell's three lucky numbers: does it hold a star,
	// where in the cell, and how fast does it twinkle?
	float starInCell = hash(id);
	vec2 starPos = vec2(hash(id + 19.19), hash(id + 7.7)) - 0.5;
	float twinkleSpeed = 1.0 + hash(id + 3.3) * 3.0;

	float dist = length(cell - starPos * 0.6); // stay inside the cell
	float star = 1.0 - smoothstep(0.01, 0.08, dist);
	star *= step(0.92, starInCell); // only about 8 in 100 cells shine

	// Twinkle: brightness waves, each star on its own clock.
	star *= 0.6 + 0.4 * sin(iTime * twinkleSpeed + starInCell * 40.0);

	vec3 colour = mix(vec3(0.02, 0.03, 0.10), vec3(0.10, 0.08, 0.25), uv.y);
	colour += vec3(1.0, 0.95, 0.85) * star;

	fragColor = vec4(colour, 1.0);
}
