#version 330

#define BLINK_THRESHOLD 0.5

uniform float time;
uniform float last_moved;

float map01(float x);

void main() {
    float t = time - last_moved;
    float blinking_state = map01(sin(5 * t));
    bool threshold = t < BLINK_THRESHOLD;

    /* mix() is a built-in GLSL function that performs linear interpolation between two values based on a third value. In this case, the third value is the result of the conversion of threshold (a boolean) to a float using float(threshold). When threshold is true, the float value becomes 1.0, and when threshold is false, the float value becomes 0.0. The mix() function then smoothly interpolates between blinking_state and 1.0 based on the float value, effectively selecting the value you desire without using explicit conditionals. */
    gl_FragColor = vec4(1.0) * mix(blinking_state, 1.0, float(threshold));
}