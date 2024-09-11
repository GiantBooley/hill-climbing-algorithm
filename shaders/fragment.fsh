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
uniform mat4 trans;
uniform int binarySearchIterations;

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

mat3 adj(mat3 m) { // Compute the adjugate of m
	return mat3(
		m[1][1]*m[2][2]-m[2][1]*m[1][2], m[2][0]*m[1][2]-m[1][0]*m[2][2], m[1][0]*m[2][1]-m[2][0]*m[1][1],
		m[2][1]*m[0][2]-m[0][1]*m[2][2], m[0][0]*m[2][2]-m[2][0]*m[0][2], m[2][0]*m[0][1]-m[0][0]*m[2][1],
		m[0][1]*m[1][2]-m[1][1]*m[0][2], m[1][0]*m[0][2]-m[0][0]*m[1][2], m[0][0]*m[1][1]-m[1][0]*m[0][1]
	);
}
mat3 basisToPoints(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {
	mat3 m = mat3(
		x1, x2, x3,
		y1, y2, y3,
		1.,  1.,  1.
	);
	vec3 v = adj(m) * vec3(x4, y4, 1.);
	return m * mat3(
		v[0], 0., 0.,
		0., v[1], 0.,
		0., 0., v[2]
	);
}
mat3 general2DProjection(
	float x1s, float y1s, float x1d, float y1d,
	float x2s, float y2s, float x2d, float y2d,
	float x3s, float y3s, float x3d, float y3d,
	float x4s, float y4s, float x4d, float y4d
) {
	mat3 s = basisToPoints(x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s);
	mat3 d = basisToPoints(x1d, y1d, x2d, y2d, x3d, y3d, x4d, y4d);
	return d * adj(s);
}
mat4 transform2d(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {
	float w = 1.;
	float h = 1.;
	mat3 t = general2DProjection(
		0., 0., x1, y1,
		w , 0., x2, y2,
		0., h , x3, y3,
		w , h , x4, y4
	);
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			t[x][y] /= t[2][2];
		}
	}
	return mat4(
		t[0][0], t[0][1], 0., t[0][2],
		t[1][0], t[1][1], 0., t[1][2],
		0.     , 0.     , 1., 0.     ,
		t[2][0], t[2][1], 0., t[2][2]);
}

vec4 doPixel(vec2 uv) {
	uv.x = mix(aabbl, aabbr, uv.x);
	uv.y = mix(aabbb, aabbt, uv.y);
	//uv = vec2(mix(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), mix(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));
	//uv = vec2(inverseLerp(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), inverseLerp(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));

	uv = uv * 2. - 1.;

	mat4 perspectiveTransform = transform2d(perspA.x, perspA.y, perspB.x, perspB.y, perspC.x, perspC.y, perspD.x, perspD.y);
	vec4 transformed = vec4(uv, 0., 1.);
	transformed = perspectiveTransform * transformed;
	uv = (transformed.xy) / transformed.w;

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
