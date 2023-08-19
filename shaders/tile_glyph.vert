#version 330 core

#define FONT_WIDTH          128.0
#define FONT_HEIGHT         64.0
#define FONT_COLS           18.0
#define FONT_ROWS           7.0
#define FONT_CHAR_WIDTH     (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT    (FONT_HEIGHT / FONT_ROWS)

uniform float time;
uniform vec2 resolution;
uniform vec2 camera;
uniform vec2 scale;

layout(location = 0) in ivec2   tile;
layout(location = 1) in int     ch;
layout(location = 2) in vec4    fg_color;
layout(location = 3) in vec4    bg_color;

out vec2 uv;
out vec4 glyph_fg_color;
out vec4 glyph_bg_color;
flat out int glyph_ch;

vec2 camera_project(vec2 point);

void main() {
    uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    vec2 char_size = vec2(FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT);
    float intensity = 5.0;

    // vec2 lsd = vec2(cos(time + tile.x), sin(time + tile.y)) * intensity;
    // vec2 pos = tile * char_size * scale + lsd;
    vec2 pos = tile * char_size * scale;

    gl_Position = vec4(camera_project(uv * char_size * scale + pos), 0.0, 1.0);

    glyph_ch = ch;
    glyph_fg_color = fg_color;
    glyph_bg_color = bg_color;
}
