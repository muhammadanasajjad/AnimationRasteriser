#version 430 core

struct ProjectedTriangle {
    vec2 position;
};

// TODO: move into projectedTrianglesBuffer
int projectedTrianglesCount = 250;
layout(std430, binding=0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

in vec2 textureCoords;

out vec4 FragColor;

void main() {
    float colour = 0.0f;
    float minRadius = 0.00f;
    float maxRadius = 0.06f;
    for (int i = 0; i < projectedTrianglesCount; i++) {
        vec2 diff = textureCoords - projectedTriangles[i].position;
        float distSquared = dot(diff, diff);
        float inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        colour = max(colour, inside);
    }
    FragColor = vec4(vec3(colour), 1.0f);
}
