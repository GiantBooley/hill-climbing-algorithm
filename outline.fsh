#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform vec3 col;

void main() {
	if (abs(texcoord.x - 0.5) < 0.45 && abs(texcoord.y - 0.5) < 0.45) discard;
	FragColor = vec4(col, 1.);
}
