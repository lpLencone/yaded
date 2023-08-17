#version 330 core

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

uniform sampler2D   font;
uniform float       time;
uniform vec2        resolution;

in vec2 uv;
in vec2 glyph_uv_pos;
in vec2 glyph_uv_size;
in vec4 glyph_fg_color;
in vec4 glyph_bg_color;


vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(
        abs(mod(6.0 * c.x + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

void main() {
    float x = glyph_uv_pos.x;
    float y = glyph_uv_pos.y;
    vec2 t = glyph_uv_pos + glyph_uv_size * uv;

    vec4 tc = texture(font, t);
    vec2 frag_uv = gl_FragCoord.xy / resolution;
    vec4 rainbow = vec4(
        hsl2rgb(
            vec3(0.5 * time + frag_uv.x * frag_uv.y * (sin(x) + cos(y)), 
                 0.5, 0.5)
        ), 
        1.0
    );

    gl_FragColor = glyph_bg_color * (1.0 - tc.x) + tc.x * glyph_fg_color * rainbow;
}

// vec4 rainbow = vec4(
//         hsl2rgb(
//             vec3(
//                 time + frag_uv.x * frag_uv.y, 
//                 map01(sin((x / 5) * time + frag_uv.x * frag_uv.y)), 
//                 map01(cos((y / 5) * time + sin(10 * frag_uv.x) + cos(10 * frag_uv.y)))
//             )
//         ), 
//         1.0
//     );

// vec4 rainbow = vec4(
//         hsl2rgb(
//             vec3(0.5 * time + frag_uv.x * frag_uv.y * (sin(x) + cos(y)), 
//                  0.5, 0.5)
//         ), 
//         1.0
//     );