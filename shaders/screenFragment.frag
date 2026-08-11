#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
    
    float depths[3];

    int materialIndex;
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

float ANTIALIASING_SCALE = 0.00005;

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

    vec2 v0 = B - A;
    vec2 v1 = C - A;
    vec2 v2 = P - A;

    float denom = v0.x * v1.y - v0.y * v1.x;
    if (denom == 0.0) {
        w1 = -1.0;
        w2 = -1.0;
        return;
    }
    float invDenom = 1.0 / denom;

    float wB = (v2.x * v1.y - v2.y * v1.x) * invDenom;
    float wC = (v0.x * v2.y - v0.y * v2.x) * invDenom;

    w1 = 1.0 - wB - wC;
    w2 = wB;
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
    float currentDepth = 1e4;
    
    for (int i = start; i < end; i++) {
        int triIdx = tileTriangles[i];
        ProjectedTriangle tri = projectedTriangles[triIdx];
        Material material = materials[tri.materialIndex];
        
        //if (uv.x < tri.min.x || uv.x > tri.max.x || uv.y < tri.min.y || uv.y > tri.max.y) continue;
        
        float w1 = 0;
        float w2 = 0;
        getBarycentric(tri, uv, w1, w2);
        float w3 = 1.0 - w1 - w2;
        
        float invDepth =
            w1 / tri.depths[0] +
            w2 / tri.depths[1] +
            w3 / tri.depths[2];

        float depth = 1.0 / invDepth;
        
        float inside = smoothstep(-ANTIALIASING_SCALE, 0.0, w1) * smoothstep(-ANTIALIASING_SCALE, 0.0, w2) * smoothstep(-ANTIALIASING_SCALE, 0.0, w3);
        inside = clamp(inside, 0.0, 1.0);
        if (depth < currentDepth && inside > 0) {
            currentDepth = depth;
            colour = material.colour * inside;
        }
    }
    
    FragColor = colour;
}
