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
uniform mat4 projMat;

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
	uv = (projMat * vec4(uv, 0., 1.)).xy;
	uv = uv * 2. - 1.;
	float r = length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = normalize(uv) * r;

	uv = (uv + 1.) * 0.5;
	vec4 color = texture(tex, uv);
	color.rgb *= col;

	FragColor = color;
}
