#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;

    vec2 min;
    vec2 max;

    vec2 padding;
};

uniform int projectedTriangleCount;
uniform int tileRows;
uniform int tileColumns;
layout(std430, binding=0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

layout(std430, binding = 2) buffer TileCount {
    int tileCounts[];
};

layout(std430, binding = 3) buffer TileOffsets {
    int tileOffsets[];
};

layout(std430, binding = 5) buffer TileTriangles {
    int tileTriangles[];
};

float ANTIALISING_SCALE = 0.0001;

in vec2 textureCoords;

out vec4 FragColor;


// 1 -> true, 0 -> false
float rightOfLine(vec2 p1, vec2 p2, vec2 point) {
    vec2 perpendicular = p2 - p1;
    perpendicular = vec2(-perpendicular.y, perpendicular.x);
    vec2 diff = point - p1;
    
    return clamp(dot(perpendicular, diff) * (1.0 / ANTIALISING_SCALE) + 0.5, 0.0, 1.0);
}

void main() {
    vec2 uv = textureCoords * 2.0 - 1.0;
    uv.y = -uv.y;

    ivec2 tile = ivec2(int(textureCoords.x * tileColumns), int(textureCoords.y * tileRows));
    int tileIdx = int(tile.y) * tileColumns + int(tile.x);
    int countInTile = tileCounts[tileIdx];
    int start = tileOffsets[tileIdx];
    int end = tileOffsets[tileIdx + 1];
    
    float minRadius = 0.00;
    float maxRadius = 0.06;
    
    float colour = 0.0;
    float redColour = 0.0;
    for (int i = start; i < end; i++) {
        int triIdx = tileTriangles[i];
        vec2 diff = uv - projectedTriangles[triIdx].p1;
        float distSquared = dot(diff, diff);
        
        float inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        redColour = max(redColour, inside);
        
        diff = uv - projectedTriangles[triIdx].p2;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        redColour = max(redColour, inside);
        
        diff = uv - projectedTriangles[triIdx].p3;
        distSquared = dot(diff, diff);
        
        inside = smoothstep(maxRadius*maxRadius, minRadius*minRadius, distSquared);
        redColour = max(redColour, inside);
    }
    
    for (int i = start; i < end; i++) {
        int triIdx = tileTriangles[i];
        ProjectedTriangle tri = projectedTriangles[triIdx];
        float right1 = rightOfLine(tri.p1, tri.p2, uv);
        float right2 = rightOfLine(tri.p2, tri.p3, uv);
        float right3 = rightOfLine(tri.p3, tri.p1, uv);
        float inside = right1 * right2 * right3;
        inside += (1.0-right1) * (1.0-right2) * (1.0-right3);
        inside = clamp(inside, 0.0, 1.0);
        colour = max(colour, inside);
    }
    
    FragColor = vec4(redColour, colour, float(countInTile) / 4, 1.0);
}
