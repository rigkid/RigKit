// 99 - MAKE YOUR OWN (YOUR SCRATCH PAD)
//
// This file is yours. Break it, fix it, make it unrecognisable -
// the other examples stay safe in their own tabs, and File then
// Save As... in the Code Editor keeps a copy of your invention as
// a .glsl file you can open again later.
//
// HOW TO START A NEW PICTURE
// 1. Decide what each pixel asks. Position (uv or p)? Distance from
//    the middle? Which cell of a grid? All pictures here begin as
//    one of those questions.
// 2. Steal ONE ingredient from another example. The recipe book:
//    - flat colour ......... 01        - circles ........... 05
//    - gradients ........... 02, 12    - checkerboards ..... 08
//    - rainbows ............ 03, 09    - grids of dots ..... 11
//    - stripes ............. 04        - stars, clouds ..... 14, 15
//    - mouse ............... 07, 18    - tunnels, mirrors .. 16, 17
// 3. Change one number at a time and guess before you look.
//
// IF THE PICTURE VANISHES OR TURNS RED-TEXTY
// - The last good picture stays while there is an error. Read the
//   error's line number, look at that line in this editor.
// - Usual culprits: a missing ; at a line end, a missing ), or a
//   whole number where a decimal belongs (write 2.0, not 2).
// - Cannot find it? Delete your last change. Still stuck? Copy the
//   starter code below afresh from this comment's little brother...
//   or pick another example tab and come back tomorrow. All real
//   shader writers do exactly that.
//
// THE STARTER BELOW slowly spins example 09's rings and tints them.
// It is deliberately boring. Your job is to ruin that, beautifully.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	// p: (0,0) in the middle, y up. uv: 0 to 1 across and up.
	vec2 uv = fragCoord / iResolution.xy;
	vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

	// One ingredient to start from: rings by distance (example 09).
	float dist = length(p);
	float wave = 0.5 + 0.5 * sin(dist * 18.0 - iTime * 1.5);

	// My colour choice - change me first!
	vec3 colour = 0.5 + 0.5 * cos(wave * 6.2831 + vec3(0.0, 2.0, 4.0));

	// A quiet edge so the picture sits nicely on the screen.
	colour *= 1.0 - smoothstep(0.55, 1.0, dist);

	// uv is unused by this starter - it waits here for your next idea.

	fragColor = vec4(colour, 1.0);
}
