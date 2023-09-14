#version 330 core

uniform sampler2D   image;
uniform vec2        resolution;
uniform float       time;

in vec2 out_uv;
in vec4 out_color;

float map01(float x) {
    return (x + 1) / 2;
}

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(
        abs(mod(6.0 * c.x + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

void main() {
    vec2 frag_uv = gl_FragCoord.xy / resolution;

    vec4 rainbow = vec4(
        hsl2rgb(
            vec3(
                0.5 * time + frag_uv.x * frag_uv.y * (sin(out_uv.x) + cos(out_uv.y)), 
                0.5, 0.5
            )
        ), 
        1.0
    );
    
    float d = texture(image, out_uv).r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);
    
    gl_FragColor = vec4(rainbow.rgb, alpha);
}
