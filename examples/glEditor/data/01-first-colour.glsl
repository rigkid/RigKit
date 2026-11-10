// 01 - FIRST COLOUR
//
// WHAT YOU WILL SEE
// The whole picture in one flat colour. Every shader in this app is a set
// of instructions that runs once for EVERY dot (pixel) on the screen.
// This one gives every pixel the same answer.
//
// TRY THIS
// 1. Change the three numbers in vec3(...) below. Numbers go from 0.0
//    (none of that colour) to 1.0 (as much as possible).
//    vec3(1.0, 0.0, 0.0) is red. vec3(0.0, 0.0, 0.0) is black.
//    vec3(1.0, 1.0, 1.0) is white. Can you make purple? Gold?
// 2. Make one number bigger than 1.0, like 2.0. What happens?
// 3. Swap two numbers around. Guessing first, then looking, is the game.
//
// THE ONE BIG IDEA
// A colour is three numbers: how much red, green and blue light to use.
// Mix the lights and your eyes see every other colour.

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	// fragCoord is "which pixel am I?" (x across, y up, in pixels).
	// This first shader ignores it - every pixel gets the same colour.

	// vec3 means "three numbers in a row": red, green, blue.
	vec3 colour = vec3(1.0, 0.45, 0.1); // a warm orange

	// vec4 is four numbers: red, green, blue, and "alpha" (how solid
	// the pixel is). 1.0 means fully solid. The picture is finished
	// when we write our answer into fragColor.
	fragColor = vec4(colour, 1.0);
}
