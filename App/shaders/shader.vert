#version 400

layout(location = 0) in vec3 vertex_position;

out vec3 colour;

void main() {
    colour = vec3(1.0, 0.0, 0.0);
    gl_Position = vec4(vertex_position, 1.0);
}
