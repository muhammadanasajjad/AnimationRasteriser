#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
};

// TODO: move into projectedTrianglesBuffer
uniform int projectedTriangleCount;
layout(std430, binding=0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

float ANTIALISING_SCALE = 0.001f;

in vec2 textureCoords;

out vec4 FragColor;


// 1 -> true, 0 -> false
float rightOfLine(vec2 p1, vec2 p2, vec2 point) {
    vec2 perpendicular = p2 - p1;
    perpendicular = vec2(-perpendicular.y, perpendicular.x);
    vec2 diff = point - p1;
    
    return smoothstep(-ANTIALISING_SCALE / 2f, ANTIALISING_SCALE / 2f, dot(perpendicular, diff));
}

void main() {
    float minRadius = 0.00f;
    float maxRadius = 0.06f;
    
    float colour = 0.0f;
    for (int i = 0; i < projectedTriangleCount; i++) {
        vec2 diff = textureCoords - projectedTriangles[i].p1;
        float distSquared = dot(diff, diff);
        
        float inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        colour = max(colour, inside);
        
        diff = textureCoords - projectedTriangles[i].p2;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        colour = max(colour, inside);
        
        diff = textureCoords - projectedTriangles[i].p3;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        colour = max(colour, inside);
    }
    
    for (int i = 0; i < projectedTriangleCount; i++) {
        ProjectedTriangle tri = projectedTriangles[i];
        float right1 = rightOfLine(tri.p1, tri.p2, textureCoords);
        float right2 = rightOfLine(tri.p2, tri.p3, textureCoords);
        float right3 = rightOfLine(tri.p3, tri.p1, textureCoords);
        float inside = right1 * right2 * right3;
        inside += (1f-right1) * (1f-right2) * (1f-right3);
        colour = max(colour, inside);
    }
    
    FragColor = vec4(vec3(colour), 1.0f);
}
