#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform vec3 col;

void main() {
	float amount = 1. - 2. * abs(texcoord.x - 0.5);
	FragColor = vec4(col * amount, 1.);
}
