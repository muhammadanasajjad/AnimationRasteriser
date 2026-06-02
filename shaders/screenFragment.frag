#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
};

uniform int projectedTriangleCount;
layout(std430, binding=0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

float ANTIALISING_SCALE = 0.001;

in vec2 textureCoords;

out vec4 FragColor;


// 1 -> true, 0 -> false
float rightOfLine(vec2 p1, vec2 p2, vec2 point) {
    vec2 perpendicular = p2 - p1;
    perpendicular = vec2(-perpendicular.y, perpendicular.x);
    vec2 diff = point - p1;
    
    return smoothstep(-ANTIALISING_SCALE / 2.0, ANTIALISING_SCALE / 2.0, dot(perpendicular, diff));
}

void main() {
    vec2 uv = textureCoords * 2.0 - 1.0;
    uv.y = -uv.y;
    
    float minRadius = 0.00;
    float maxRadius = 0.06;
    
    float colour = 0.0;
    for (int i = 0; i < projectedTriangleCount; i++) {
        vec2 diff = uv - projectedTriangles[i].p1;
        float distSquared = dot(diff, diff);
        
        float inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        //colour = max(colour, inside);
        
        diff = uv - projectedTriangles[i].p2;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        //colour = max(colour, inside);
        
        diff = uv - projectedTriangles[i].p3;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        //colour = max(colour, inside);
    }
    
    for (int i = 0; i < projectedTriangleCount; i++) {
        ProjectedTriangle tri = projectedTriangles[i];
        float right1 = rightOfLine(tri.p1, tri.p2, uv);
        float right2 = rightOfLine(tri.p2, tri.p3, uv);
        float right3 = rightOfLine(tri.p3, tri.p1, uv);
        float inside = right1 * right2 * right3;
        inside += (1.0-right1) * (1.0-right2) * (1.0-right3);
        colour = max(colour, inside);
    }
    
    FragColor = vec4(vec3(colour), 1.0);
}
