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
uniform int aaRes;
uniform vec2 resolution;
uniform vec2 perspA;
uniform vec2 perspB;
uniform vec2 perspC;
uniform vec2 perspD;
uniform int binarySearchIterations;

float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * pow(r, 3.) + b * pow(r, 2.) + c * r + d) * r;
}
float inverseLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}
float inverseLensDistortion(float r, float a, float b, float c, float d) {
	float answer = 0.;

	float start = 0.;
	float end = 10.;

	for (int i = 0; i < binarySearchIterations; i++) {
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
vec4 doPixel(vec2 uv) {
	uv.x = mix(aabbl, aabbr, uv.x);
	uv.y = mix(aabbb, aabbt, uv.y);
	uv = vec2(mix(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), mix(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));
	//uv = vec2(inverseLerp(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), inverseLerp(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));

	uv = uv * 2. - 1.;
	uv.x *= ratio;
	float r = length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = normalize(uv) * r;

	uv.x /= ratio;
	uv = (uv + 1.) * 0.5;


	return texture(tex, uv);
}

void main() {
	float w = 1. / resolution.x / float(aaRes);
	float h = 1. / resolution.y / float(aaRes);
	vec4 color = vec4(0.);
	for (int x = 0; x < aaRes; x++) {
		for (int y = 0; y < aaRes; y++) {
			color += doPixel(texcoord + vec2(w * float(x), h * float(y)));
		}
	}
	color /= float(aaRes * aaRes);
	color.rgb *= col;

	FragColor = color;
}
