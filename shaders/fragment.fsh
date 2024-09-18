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
uniform mat3 trans;
uniform int binarySearchIterations;
uniform bool combineMosaic;
uniform bool showTransform;
uniform ivec2 grid;


float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * r * r + b * r + c) * r * r + d * r;//6 multiplications
}
float inverseLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}
float inverseLensDistortion(float r, float a, float b, float c, float d) {
	if (a == 0. && b == 0. && c == 0. && d == 1.) return r;
	float answer = 0.;

	float start = 0.;
	float end = 10.;

	for (int i = 0; i < binarySearchIterations; i++) {
		answer = (start + end) * 0.5;

		if (lensDistortion(answer, a, b, c, d) < r) {
			start = answer;
		} else {
			end = answer;
		}
	}

	return answer;
}


vec4 doPixel(vec2 uv, float left, float right, float bottom, float top) {
	uv.x = mix(left, right, uv.x);
	uv.y = mix(bottom, top, uv.y);
	//uv = vec2(mix(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), mix(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));
	//uv = vec2(inverseLerp(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), inverseLerp(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));

	if (showTransform) {
		vec3 transformed = vec3(uv, 1.);
		transformed = trans * transformed;
		uv = (transformed.xy) / transformed.z;
	}

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
	float h = 1. / resolution.y / float(aaRes);	vec4 color = vec4(0.);
	for (int x = 0; x < aaRes; x++) {
		for (int y = 0; y < aaRes; y++) {
			if (combineMosaic) {
				float howmany = 0.;
				vec3 currentColor = vec3(0.);
				for (int ecks = 0; ecks < grid.x; ecks++) {
					for (int why = 0; why < grid.y; why++) {
						vec4 pixelColor = doPixel(
							texcoord + vec2(w * float(x), h * float(y)),
							mix(float(ecks) / float(grid.x), float(ecks + 1) / float(grid.x), aabbl),
							mix(float(ecks) / float(grid.x), float(ecks + 1) / float(grid.x), aabbr),
							mix(float(why) / float(grid.y), float(why + 1) / float(grid.y), aabbb),
							mix(float(why) / float(grid.y), float(why + 1) / float(grid.y), aabbt)
						);
						currentColor += pixelColor.rgb * pixelColor.a;
						howmany += pixelColor.a;
					}
				}
				currentColor /= howmany;
				color += vec4(currentColor, howmany > 0. ? 1. : 0.);
			} else {
				color += doPixel(texcoord + vec2(w * float(x), h * float(y)), aabbl, aabbr, aabbb, aabbt);
			}
		}
	}
	color /= float(aaRes * aaRes);
	color.rgb *= col;

	FragColor = color;
}
