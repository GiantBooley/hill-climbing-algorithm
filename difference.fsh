#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float aabb[4];

void main() {
	vec3 color1 = texture(tex1, vec2(mix(aabb[0], aabb[1], texcoord.x), mix(aabb[2], aabb[3], texcoord.y))).rgb;
	vec3 color2 = texture(tex2, texcoord).rgb;
	FragColor = vec4(abs(color2 - color1), 1.);
}
