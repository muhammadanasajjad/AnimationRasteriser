#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;

    vec2 min;
    vec2 max;

    int materialIndex;
    float padding;
};

struct Material {
    vec4 colour;
    int textureIndex;
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

layout(std430, binding = 6) buffer Materials {
    Material materials[];
};

float ANTIALIASING_SCALE = 0.01;

in vec2 textureCoords;

out vec4 FragColor;


// 1 -> true, 0 -> false
float rightOfLine(vec2 p1, vec2 p2, vec2 point) {
    vec2 perpendicular = p2 - p1;
    perpendicular = vec2(-perpendicular.y, perpendicular.x);
    vec2 diff = point - p1;

    float aaScale = perpendicular.x*perpendicular.x + perpendicular.y*perpendicular.y;
    aaScale *= ANTIALIASING_SCALE;
    aaScale = clamp(aaScale, 0.1, 0.00001);
    return clamp(dot(perpendicular, diff) * (1.0 / ANTIALIASING_SCALE) + 0.5, 0.0, 1.0);
}

void getBarycentric(ProjectedTriangle tri, vec2 P, inout float w1, inout float w2) {
    vec2 A = tri.p1;
    vec2 B = tri.p2;
    vec2 C = tri.p3;

    w1 = A.x * (C.y - A.y) + (P.y - A.y) * (C.x - A.x) - P.x * (C.y - A.y);
    w1 *= 1.0 / ( (B.y - A.y) * (C.x - A.x) - (B.x - A.x) * (C.y - A.y) );

    w2 = P.y - A.y - w1 * (B.y - A.y);
    w2 *= 1.0 / ( C.y - A.y );
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
    
    vec4 colour = vec4(0.0);
    
    for (int i = start; i < end; i++) {
        int triIdx = tileTriangles[i];
        ProjectedTriangle tri = projectedTriangles[triIdx];
        Material material = materials[tri.materialIndex];
        
        //if (uv.x < tri.min.x || uv.x > tri.max.x || uv.y < tri.min.y || uv.y > tri.max.y) continue;
        
        float w1 = 0;
        float w2 = 0;
        getBarycentric(tri, uv, w1, w2);
        
        float inside = smoothstep(0.0, ANTIALIASING_SCALE, w1) * smoothstep(0.0, ANTIALIASING_SCALE, w2) * smoothstep(0.0, ANTIALIASING_SCALE, 1.0 - (w1 + w2));
        inside = clamp(inside, 0.0, 1.0);
        colour = max(colour, material.colour * inside);
    }
    
    FragColor = colour;
}
