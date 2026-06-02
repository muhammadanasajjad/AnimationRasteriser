#version 430 core

layout (location = 0) in vec3 aPos;

out vec2 textureCoords;

void main() {
    textureCoords = vec2(0.0, 1.0) - vec2(-aPos.x - 1.0, aPos.y + 1.0) / 2.0;
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
