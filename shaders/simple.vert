#version 330 core

uniform float   time;
uniform vec2    resolution;
uniform vec2    camera;
uniform vec2    scale;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 pos;
layout(location = 2) in vec4 color;

out vec4 out_color;
out vec2 out_uv;

float map01(float x) {
    return (x + 1) / 2;
}

vec2 camera_project(vec2 point) 
{
    vec2 cam_uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    float lsd = 0.975 + map01(
        sin(2 * (
            sin(map01(cos(time)) * (1.0 + cam_uv.x)) + 
            cos(map01(sin(time)) * (1.0 + cam_uv.y))
        ))
    ) / 20.0;

    return 2.0 * (point - camera) * scale * lsd / resolution;
}


void main() {
    out_color   = color;
    out_uv      = uv;

    gl_Position = vec4(camera_project(pos), 0, 1);
}
