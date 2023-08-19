#version 330 core

uniform vec2 resolution;
uniform vec2 camera;
uniform vec2 pos;
uniform float height;
uniform float width;
uniform float time;

out vec2 uv;

vec2 camera_project(vec2 point);

void main() {
    uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));

    gl_Position = vec4(
        camera_project(uv * vec2(width, height) + pos), 
        0.0, 1.0
    );
}