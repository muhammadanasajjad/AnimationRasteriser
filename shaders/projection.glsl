#version 430 core

struct WolrdTriangle {
    vec3 p1;
    vec3 p2;
    vec3 p3;
};

uniform int triangleCount;
layout(std430, binding=1) buffer WorldTrianglesBuffer {
    WolrdTriangle worldTriangles[];
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    
}
