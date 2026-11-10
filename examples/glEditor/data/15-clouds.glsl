// 15 - CLOUDS (SMOOTH NOISE)
//
// WHAT YOU WILL SEE
// Soft drifting clouds. Under the softness is a grid of random numbers
// (example 14's hash) - blurred until the grid disappears.
//
// TRY THIS
// 1. Cloud amount: mix edge 0.45 and 0.75 in the sky line. Lower
//    numbers, more cloud. Try 0.2 and 0.5 for a storm.
// 2. Wind speed: iTime * 0.03. And wind direction: change the vec2 to
//    vec2(0.03, 0.01) * iTime to drift diagonally.
// 3. Detail: the loop runs 4 times (i < 4). Try 2 (blobby) and 6
//    (fluffier - but slower; watch the FPS counter).
// 4. Sunset clouds: sky blue vec3(0.35, 0.6, 1.0) to orange, and the
//    cloud colour to pink.
// 5. Fog idea: use ONE noise layer (loop once) in grey over example
//    12's sunset. Layers again - you know this trick now.
//
// THE ONE BIG IDEA
// Blur random numbers smoothly (smoothstep between cell corners),
// then add copies at shrinking sizes. Big soft blobs + medium blobs +
// small blobs = clouds. Piling up sizes is called fbm, and the loop
// below is the whole recipe - many famous shaders start right here.

// Scramble a cell number into a 0-to-1 number that looks random.
float hash(vec2 cell) {
	return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
}

// One smooth-blurred layer of random numbers at position q.
float noise(vec2 q) {
	vec2 id = floor(q);	 // the four corners around q
	vec2 f = fract(q);	 // how far across its cell q sits
	vec2 u = f * f * (3.0 - 2.0 * f); // smooth the fraction (soft step)

	float bottom = mix(hash(id), hash(id + vec2(1.0, 0.0)), u.x);
	float top = mix(hash(id + vec2(0.0, 1.0)), hash(id + vec2(1.0, 1.0)), u.x);
	return mix(bottom, top, u.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;

	// Drift sideways with the wind.
	vec2 q = uv * 4.0 + vec2(iTime * 0.03, 0.0);

	// Add layers: each round doubles the detail and halves the weight.
	float clouds = 0.0;
	float weight = 0.5;
	vec2 layer = q;
	for (int i = 0; i < 4; i++) {
		clouds += noise(layer) * weight;
		layer *= 2.0;	 // next layer has twice the detail
		weight *= 0.5; // ...and half the say
	}

	// clouds runs about 0 to 1. Turn it into sky-and-cloud colour.
	float cover = smoothstep(0.45, 0.75, clouds);
	vec3 colour = mix(vec3(0.35, 0.6, 1.0), vec3(1.0), cover);

	fragColor = vec4(colour, 1.0);
}
