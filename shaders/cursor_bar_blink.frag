#version 330

uniform float time;

in vec2 uv;

void main() {
    gl_FragColor = sin(3 * time) * vec4(1.0);
}