#pragma sokol @ctype vec2 glm::vec2
#pragma sokol @ctype vec3 glm::vec3
#pragma sokol @ctype vec4 glm::vec4
#pragma sokol @ctype mat4 glm::mat4

#pragma sokol @vs vs
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec2 v_texCoord;
out vec4 v_color;

uniform vs_params {
    mat4 u_trans;
};

void main() {
    v_texCoord = a_uv;
    v_color = a_color;
    gl_Position = u_trans * vec4(a_pos.x, a_pos.y, 0.0, 1.0);
}
#pragma sokol @end

#pragma sokol @fs fs
in vec2 v_texCoord;
in vec4 v_color;
out vec4 color;

uniform sampler2D u_tex;

void main() {
    color = v_color * texture(u_tex, v_texCoord);
}
#pragma sokol @end

#pragma sokol @program sprite vs fs