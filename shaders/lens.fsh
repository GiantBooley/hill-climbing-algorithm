#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex;
uniform vec3 col;

void main() {
	vec4 color = texture(tex, texcoord);
	color.rgb *= col;

	FragColor = color;
}
