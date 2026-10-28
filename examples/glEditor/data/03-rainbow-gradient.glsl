// 03 - RAINBOW GRADIENT (IT MOVES!)
//
// WHAT YOU WILL SEE
// Smooth rainbow waves that drift on their own. This is the classic
// first Shadertoy picture.
//
// TRY THIS
// 1. Take iTime out of the cos(...) line. The picture freezes. Put it
//    back - iTime is a clock, in seconds, and it makes things move.
// 2. Change iTime to iTime * 3.0 (faster) or iTime * 0.2 (sleepy).
// 3. Change uv.xyx to uv.yxy. Which way do the waves run now?
// 4. Change uv.xyx to uv.xyx * 3.0. More rainbows fit on the screen.
// 5. The numbers vec3(0.0, 2.0, 4.0) start the three colour waves at
//    different places. Try vec3(0.0, 0.0, 0.0). Less rainbow, right?
//
// THE ONE BIG IDEA
// cos(...) is a wave: it goes 1, 0, -1, 0, 1... forever. The trick
// 0.5 + 0.5 * cos(...) squashes that wave into 0.0 to 1.0, which is
// exactly the range a colour needs. Three waves, started at different
// times, make a rainbow.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;

	// One cos wave per colour. Red, green and blue each get the wave
	// shifted a bit (0.0, 2.0, 4.0), so they peak in different places -
	// and your eyes see that as different colours sweeping across.
	// Adding iTime moves the whole wave along as the clock ticks.
	vec3 colour = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));

	fragColor = vec4(colour, 1.0);
}
