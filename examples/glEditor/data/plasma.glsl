// Shadertoy-style template — soft plasma
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 uv = fragCoord / iResolution.xy;
	float t = iTime;
	float v = sin(uv.x * 10.0 + t);
	v += sin(uv.y * 10.0 + t * 1.3);
	v += sin((uv.x + uv.y) * 8.0 + t * 0.7);
	v += sin(length(uv - 0.5) * 14.0 - t);
	v *= 0.25;
	vec3 col = 0.5 + 0.5 * cos(3.14159 * v + vec3(0.0, 2.0, 4.0));
	fragColor = vec4(col, 1.0);
}
