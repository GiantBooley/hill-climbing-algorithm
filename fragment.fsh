#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex;

void main() {
	vec4 color = texture(tex, texcoord);
	FragColor = color;
}
