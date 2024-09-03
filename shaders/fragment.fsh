#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex;
uniform vec3 col;
uniform float a;
uniform float b;
uniform float c;
uniform float d;

uniform mat4 modelMat;
uniform float aabbl;
uniform float aabbr;
uniform float aabbb;
uniform float aabbt;
uniform float ratio;

float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * pow(r, 3.) + b * pow(r, 2.) + c * r + d) * r;
}
float inverseLensDistortion(float r, float a, float b, float c, float d) {
	float answer = 0.;

	float start = 0.;
	float end = 10.;

	for (int i = 0; i < 32; i++) {
		float mid = (start + end) * 0.5;
		float l = lensDistortion(mid, a, b, c, d);
		answer = mid;

		if (l < r) {
			start = mid;
			answer = mid;
		} else {
			end = mid;
		}
	}

	return answer;
}

void main() {
	vec2 uv = texcoord;
	uv.x = mix(aabbl, aabbr, uv.x);
	uv.y = mix(aabbb, aabbt, uv.y);
	uv = uv * 2. - 1.;
	uv.x *= ratio;
	float r = length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = normalize(uv) * r;

	uv.x /= ratio;
	uv = (uv + 1.) * 0.5;
	vec4 color = texture(tex, uv);
	color.rgb *= col;

	FragColor = color;
}
