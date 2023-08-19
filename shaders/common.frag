#version 330 core

float map01(float x) {
    return (x + 1) / 2;
}

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(
        abs(mod(6.0 * c.x + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}