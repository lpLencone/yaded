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
    float lsd = 0.95 + map01(
        sin(2 * (
            sin(map01(cos(time)) * (1.0 + uv.x)) + 
            cos(map01(sin(time)) * (1.0 + uv.y))
        ))
    ) / 10.0;

    return 2.0 * (point - camera) * scale * lsd / resolution;
}

// float lsd = (1.0 + sin(time + sin(uv.x) + cos(uv.y)));
// float lsd = (1.0 + map01(sin(time + sin(uv.x) + cos(uv.y))));
