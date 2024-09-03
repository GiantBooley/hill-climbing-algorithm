#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform vec3 col;

void main() {
	FragColor = vec4(col, 1.);
}
