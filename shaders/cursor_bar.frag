#version 330

#define PERIOD 0.5
#define BLINK_THRESHOLD 0.5

uniform float   time;
uniform float   last_moved;

void main() {
    bool c1 = time - last_moved < BLINK_THRESHOLD;
    bool c2 = bool(int(floor(time / PERIOD)) % 2);
    
    gl_FragColor = vec4(1.0) * int(c1 || c2);
}