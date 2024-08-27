#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex1;
uniform sampler2D tex2;

void main() {
	vec3 color1 = texture(tex1, texcoord).rgb;
	vec3 color2 = texture(tex2, texcoord).rgb;
	FragColor = vec4(abs(color2 - color1), 1.);
}
