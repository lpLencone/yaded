#version 330 core

float map01(float x) {
    return (x + 1) / 2;
}

uniform vec2    resolution;
uniform vec2    camera;
uniform vec2    scale;

vec2 camera_project(vec2 point) 
{
    return 2.0 * (point - camera) * scale / resolution;
}
