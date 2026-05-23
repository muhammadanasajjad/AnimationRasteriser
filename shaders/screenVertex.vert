#version 430 core

layout (location = 0) in vec3 aPos;

out vec2 textureCoords;

void main() {
    textureCoords = vec2(0f, 1f) - vec2(-aPos.x - 1f, aPos.y + 1f) / 2f;
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
