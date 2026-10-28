// 11 - POLKA DOTS
//
// WHAT YOU WILL SEE
// A neat grid of dots, each one gently pulsing, in rows of colour.
//
// TRY THIS
// 1. Change the 3.0 in p * 3.0 (both places) to 5.0 or 8.0.
// 2. Change the pulse speed iTime * 2.0, and the dot size 0.18.
// 3. Make the dots squares: replace the dist line with
//    float dist = max(abs(cell.x), abs(cell.y));
// 4. Colour by column instead of row: use id.x instead of id.y.
// 5. Chase pattern: add id.x to the pulse's sin(...) so dots ripple
//    across: sin(iTime * 2.0 + id.x + id.y)
//
// THE ONE BIG IDEA
// floor and fract are a team. floor(uv * 3.0) names the cell a pixel
// lives in ((0,0), (1,0), ...), and fract(uv * 3.0) says where the
// pixel sits INSIDE its cell (0 to 1). Draw the same small picture
// in every cell and you get a pattern.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	vec2 id = floor(p * 3.0);		 // which cell am I in?
	vec2 cell = fract(p * 3.0) - 0.5; // -0.5 to 0.5 inside the cell

	// A circle in the middle of every cell, breathing in and out.
	// Adding id.y makes each row pulse at a different time.
	float size = 0.18 + 0.06 * sin(iTime * 2.0 + id.y * 1.7);
	float dotShape = 1.0 - smoothstep(size - 0.03, size, length(cell));

	// Each row gets its own rainbow colour (example 03's trick again).
	vec3 dotColour = 0.5 + 0.5 * cos(id.y * 1.3 + vec3(0.0, 2.0, 4.0));

	vec3 colour = mix(vec3(0.07, 0.07, 0.12), dotColour, dotShape);

	fragColor = vec4(colour, 1.0);
}
