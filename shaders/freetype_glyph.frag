#version 330 core

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

uniform sampler2D   font;
uniform float       time;
uniform vec2        resolution;

in vec2 uv;
flat in int glyph_ch;
in vec4 glyph_fg_color;
in vec4 glyph_bg_color;

void main() {
    int ch = glyph_ch;
    if (ch < ASCII_DISPLAY_LOW || ASCII_DISPLAY_HIGH < ch) {
        ch = 63;
    }

    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}