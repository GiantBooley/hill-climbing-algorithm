#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform vec3 col;

void main() {
	float d = length(texcoord - 0.5);
	if (d > 0.5 || d < 0.3) discard;
	FragColor = vec4(col, 1.);
}
