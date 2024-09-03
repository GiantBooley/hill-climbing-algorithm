#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform vec3 col;
uniform vec2 size;

vec2 lensDistortion(vec2 uv) {
	return uv;
}

void main() {
	float xWidth = 5. / size.x;
	float yWidth = 5. / size.y;
	if (abs(texcoord.x - 0.5) < 0.5 - xWidth && abs(texcoord.y - 0.5) < 0.5 - yWidth) discard;
	FragColor = vec4(col, 1.);
}
