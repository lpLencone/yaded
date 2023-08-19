#version 330 core

float map01(float x) {
    return (x + 1) / 2;
}

uniform float   time;
uniform vec2    resolution;
uniform vec2    camera;
uniform vec2    scale;

vec2 camera_project(vec2 point) 
{
    vec2 uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));

    return 2.0 * (point - camera) * scale * (1.0 + map01(sin(time + sin(uv.x) + cos(uv.y)))) / resolution;
}

// return 2.0 * (point - camera) * scale * (1.0 + sin(time + sin(uv.x) + cos(uv.y))) / resolution;
