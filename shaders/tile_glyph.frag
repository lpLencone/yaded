#version 330 core

#define FONT_WIDTH              128
#define FONT_HEIGHT             64
#define FONT_COLS               18
#define FONT_ROWS               7
#define FONT_CHAR_WIDTH         (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT        (FONT_HEIGHT / FONT_ROWS)
#define FONT_CHAR_WIDTH_UV      (float(FONT_CHAR_WIDTH)  / float(FONT_WIDTH))
#define FONT_CHAR_HEIGHT_UV     (float(FONT_CHAR_HEIGHT) / float(FONT_HEIGHT))

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

uniform sampler2D   font;
uniform float       time;
uniform vec2        resolution;

in vec2 uv;
flat in int glyph_ch;
in vec4 glyph_fg_color;
in vec4 glyph_bg_color;

float map01(float x);
vec3 hsl2rgb(vec3 c);

void main() {
    int ch = glyph_ch;
    if (ch < ASCII_DISPLAY_LOW || ASCII_DISPLAY_HIGH < ch) {
        ch = 63;
    }

    int index = ch - 32;
    float x = float(index % FONT_COLS) * FONT_CHAR_WIDTH_UV;
    float y = float(index / FONT_COLS) * FONT_CHAR_HEIGHT_UV;
    vec2 pos = vec2(x, y + FONT_CHAR_HEIGHT_UV);
    vec2 size = vec2(FONT_CHAR_WIDTH_UV, -FONT_CHAR_HEIGHT_UV);

    vec2 t = pos + size * uv;

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