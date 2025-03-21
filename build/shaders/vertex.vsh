#version 300 es
precision mediump float;

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexcoord;

out vec2 texcoord;

uniform mat4 modelMat;
uniform mat4 projMat;

void main() {
    vec2 pos = aPos;
	gl_Position = projMat * modelMat * vec4(pos, 0., 1.);
	texcoord = aTexcoord;
}
