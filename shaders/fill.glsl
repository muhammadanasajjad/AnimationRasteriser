#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;

    vec2 min;
    vec2 max;
    
    float depths[3];

    int materialIndex;
    float padding;
};

layout(std430, binding = 0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

uniform int projectedTriangleCount;
uniform int tileRows;
uniform int tileColumns;

layout(std430, binding = 3) buffer TileOffsets {
    int tileOffsets[];
};

layout(std430, binding = 4) buffer TileCounters {
    int tileCounters[];
};

layout(std430, binding = 5) buffer TileTriangles {
    int tileTriangles[];
};

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= projectedTriangleCount) return;

    ProjectedTriangle tri = projectedTriangles[idx];

    ivec2 minTile = ivec2(max(0, int(ceil(((tri.min.x + 1.0) / 2.0) * tileColumns - 1.0))),
                          max(0, int(ceil(((1.0 - tri.max.y) / 2.0) * tileRows - 1.0))));
    ivec2 maxTile = ivec2(min(tileColumns - 1, int(floor(((tri.max.x + 1.0) / 2.0) * tileColumns))),
                          min(tileRows - 1, int(floor(((1.0 - tri.min.y) / 2.0) * tileRows))));

    for (int ty = minTile.y; ty <= maxTile.y; ty++) {
        for (int tx = minTile.x; tx <= maxTile.x; tx++) {
            int tileIdx = ty * tileColumns + tx;
            int base = tileOffsets[tileIdx];
            int counter = atomicAdd(tileCounters[tileIdx], 1);
            tileTriangles[base + counter] = int(idx);
        }
    }
}
