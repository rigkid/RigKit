(() => {
	const canvas = document.getElementById("field");
	if (!(canvas instanceof HTMLCanvasElement)) return;

	const gl = canvas.getContext("webgl", {
		alpha: false,
		antialias: false,
		depth: false,
		stencil: false,
		powerPreference: "low-power",
	});
	if (!gl) return;

	const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

	function compile(type, source) {
		const shader = gl.createShader(type);
		gl.shaderSource(shader, source);
		gl.compileShader(shader);
		if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
			console.warn(gl.getShaderInfoLog(shader));
			gl.deleteShader(shader);
			return null;
		}
		return shader;
	}

	const vs = compile(
		gl.VERTEX_SHADER,
		`
		attribute vec2 a_pos;
		void main() {
			gl_Position = vec4(a_pos, 0.0, 1.0);
		}
		`
	);

	const fs = compile(
		gl.FRAGMENT_SHADER,
		`
		precision mediump float;

		uniform vec2 u_res;
		uniform vec2 u_mouse;
		uniform float u_time;
		uniform float u_seed;
		uniform float u_scale;
		uniform float u_hue;
		uniform float u_warp;

		float hash(vec2 p) {
			p = fract(p * vec2(123.34 + u_seed, 456.21 - u_seed));
			p += dot(p, p + 45.32);
			return fract(p.x * p.y);
		}

		float noise(vec2 p) {
			vec2 i = floor(p);
			vec2 f = fract(p);
			float a = hash(i);
			float b = hash(i + vec2(1.0, 0.0));
			float c = hash(i + vec2(0.0, 1.0));
			float d = hash(i + vec2(1.0, 1.0));
			vec2 u = f * f * (3.0 - 2.0 * f);
			return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
		}

		float fbm(vec2 p) {
			float v = 0.0;
			float a = 0.5;
			for (int i = 0; i < 5; i++) {
				v += a * noise(p);
				p = p * 2.02 + vec2(17.1, 9.3);
				a *= 0.5;
			}
			return v;
		}

		vec2 field(vec2 p) {
			float e = 0.02;
			float dx = fbm(p + vec2(e, 0.0)) - fbm(p - vec2(e, 0.0));
			float dy = fbm(p + vec2(0.0, e)) - fbm(p - vec2(0.0, e));
			return vec2(dy, -dx) / (2.0 * e);
		}

		vec3 palette(float t) {
			vec3 ink = vec3(0.055, 0.07, 0.063);
			vec3 moss = vec3(0.12, 0.28, 0.24);
			vec3 sand = vec3(0.78, 0.68, 0.48);
			vec3 coral = vec3(0.89, 0.34, 0.18);
			t = fract(t + u_hue);
			if (t < 0.35) return mix(ink, moss, t / 0.35);
			if (t < 0.65) return mix(moss, sand, (t - 0.35) / 0.30);
			return mix(sand, coral, (t - 0.65) / 0.35);
		}

		void main() {
			vec2 uv = gl_FragCoord.xy / u_res;
			float aspect = u_res.x / max(u_res.y, 1.0);
			vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
			p *= u_scale;

			vec2 m = (u_mouse - 0.5) * vec2(aspect, 1.0);
			m *= u_scale;
			float md = length(p - m);
			float pull = u_warp / (0.35 + md * md * 2.2);
			p += normalize(p - m + vec2(0.0001)) * pull * 0.22;
			p += vec2(
				sin(u_time * 0.11 + u_seed),
				cos(u_time * 0.09 - u_seed)
			) * 0.08;

			vec2 v = field(p * 1.35 + vec2(u_seed * 3.1, -u_seed * 2.4));
			float ang = atan(v.y, v.x);
			float mag = length(v);

			float ribbons = fbm(p * 0.85 + v * 0.35 + u_time * 0.04);
			float t = ang / 6.2831853 + 0.5 + ribbons * 0.35 + mag * 0.08;
			vec3 col = palette(t);

			float grain = hash(gl_FragCoord.xy + u_time) * 0.04;
			float vignette = smoothstep(1.35, 0.25, length((uv - 0.5) * vec2(1.15, 1.0)));
			col = mix(col * 0.55, col, vignette);
			col += grain;

			float ring = exp(-md * md * 6.0) * 0.18;
			float flow = smoothstep(0.2, 1.4, mag) * 0.08;
			col += vec3(0.89, 0.34, 0.18) * (ring + flow);

			gl_FragColor = vec4(col, 1.0);
		}
		`
	);

	if (!vs || !fs) return;

	const program = gl.createProgram();
	gl.attachShader(program, vs);
	gl.attachShader(program, fs);
	gl.linkProgram(program);
	if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
		console.warn(gl.getProgramInfoLog(program));
		return;
	}
	gl.useProgram(program);

	const buf = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, buf);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
	const aPos = gl.getAttribLocation(program, "a_pos");
	gl.enableVertexAttribArray(aPos);
	gl.vertexAttribPointer(aPos, 2, gl.FLOAT, false, 0, 0);

	const uRes = gl.getUniformLocation(program, "u_res");
	const uMouse = gl.getUniformLocation(program, "u_mouse");
	const uTime = gl.getUniformLocation(program, "u_time");
	const uSeed = gl.getUniformLocation(program, "u_seed");
	const uScale = gl.getUniformLocation(program, "u_scale");
	const uHue = gl.getUniformLocation(program, "u_hue");
	const uWarp = gl.getUniformLocation(program, "u_warp");

	const seed = Math.random() * 100;
	const scale = 1.6 + Math.random() * 1.8;
	const hue = Math.random();
	const warp = 0.55 + Math.random() * 0.85;

	gl.uniform1f(uSeed, seed);
	gl.uniform1f(uScale, scale);
	gl.uniform1f(uHue, hue);
	gl.uniform1f(uWarp, warp);

	const mouse = { x: 0.5, y: 0.5 };
	const mouseTarget = { x: 0.5, y: 0.5 };

	function onPointer(clientX, clientY) {
		const rect = canvas.getBoundingClientRect();
		if (rect.width <= 0 || rect.height <= 0) return;
		mouseTarget.x = (clientX - rect.left) / rect.width;
		mouseTarget.y = 1.0 - (clientY - rect.top) / rect.height;
	}

	window.addEventListener(
		"pointermove",
		(e) => {
			onPointer(e.clientX, e.clientY);
		},
		{ passive: true }
	);

	function resize() {
		const dpr = Math.min(window.devicePixelRatio || 1, 1.5);
		const w = Math.max(1, Math.floor(canvas.clientWidth * dpr));
		const h = Math.max(1, Math.floor(canvas.clientHeight * dpr));
		if (canvas.width !== w || canvas.height !== h) {
			canvas.width = w;
			canvas.height = h;
			gl.viewport(0, 0, w, h);
		}
		gl.uniform2f(uRes, w, h);
	}

	let start = performance.now();
	let running = true;

	function frame(now) {
		if (!running) return;
		resize();

		mouse.x += (mouseTarget.x - mouse.x) * 0.08;
		mouse.y += (mouseTarget.y - mouse.y) * 0.08;

		const t = reduceMotion ? 0 : (now - start) * 0.001;
		gl.uniform1f(uTime, t);
		gl.uniform2f(uMouse, mouse.x, mouse.y);
		gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

		if (!reduceMotion) requestAnimationFrame(frame);
	}

	resize();
	requestAnimationFrame(frame);

	document.addEventListener("visibilitychange", () => {
		if (document.hidden) {
			running = false;
			return;
		}
		if (!reduceMotion && !running) {
			running = true;
			requestAnimationFrame(frame);
		}
	});
})();
