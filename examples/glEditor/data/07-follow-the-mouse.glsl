// 07 - FOLLOW THE MOUSE
//
// WHAT YOU WILL SEE
// A glowing spot that follows your mouse over the preview picture.
// Hold the mouse button down and it changes colour and pulses.
//
// TRY THIS
// 1. Just move the mouse over the picture on the left. Then hold the
//    button down. (The mouse only counts while it is over the picture.)
// 2. Change the size of the spot: the 0.25 and 0.0 in smoothstep.
// 3. Change rings speed: iTime * 4.0 in the sin(...) line.
// 4. Make TWO spots: draw a second circle at a fixed place, or at the
//    mouse reflected: vec2(-mouseP.x, mouseP.y).
// 5. Trail idea: make the spot's colour depend on how fast... too hard?
//    Try colour = distance from the centre of the screen instead.
//
// THE ONE BIG IDEA
// iMouse is the mouse position in pixels: .x and .y say where it is,
// and .z is bigger than 0 while you hold the button down (it stores
// where you clicked). Any value the computer hands you each frame -
// time, mouse, screen size - can steer the picture.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// The mouse arrives in pixels, same as fragCoord. Put it through
	// the same recentring trick so it lines up with p.
	vec2 mouseP = (iMouse.xy - 0.5 * iResolution.xy) / iResolution.y;

	float dist = length(p - mouseP);

	// A soft glow: 1 in the middle, fading to 0 by 0.25 away.
	float glow = 1.0 - smoothstep(0.0, 0.25, dist);

	// Rings running outwards from the mouse.
	float rings = 0.5 + 0.5 * sin(dist * 40.0 - iTime * 4.0);

	// Held down or not? iMouse.z is 0 until you press over the picture.
	float pressed = step(1.0, iMouse.z);
	vec3 idleColour = vec3(0.2, 0.6, 1.0);	// blue
	vec3 heldColour = vec3(1.0, 0.45, 0.1); // orange
	vec3 spotColour = mix(idleColour, heldColour, pressed);

	vec3 colour = vec3(0.03, 0.03, 0.08);		  // near-black background
	colour += spotColour * glow * (0.6 + 0.4 * rings); // add light

	fragColor = vec4(colour, 1.0);
}
