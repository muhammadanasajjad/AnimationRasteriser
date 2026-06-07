#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;

    vec2 min;
    vec2 max;

    vec2 padding;
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

    vec2 tileSize = vec2(1.0 / float(tileColumns), 1.0 / float(tileRows));

    for (int ty = 0; ty < tileRows; ty++) {
        for (int tx = 0; tx < tileColumns; tx++) {
            vec2 tileMin = vec2(tx, ty) * tileSize;
            vec2 tileMax = tileMin + tileSize;
            tileMin.x = tileMin.x * 2.0 - 1.0;
            tileMin.y = 1.0 - tileMin.y * 2.0;
            tileMax.x = tileMax.x * 2.0 - 1.0;
            tileMax.y = 1.0 - tileMax.y * 2.0;

            float hit =
                step(tileMin.x, tri.max.x) *
                step(tri.min.x, tileMax.x) *
                step(tileMax.y, tri.max.y) *
                step(tri.min.y, tileMin.y);

            if (hit > 0.0) {
                int tileIdx = ty * tileColumns + tx;
                int base = tileOffsets[tileIdx];
                int counter = atomicAdd(tileCounters[tileIdx], 1);
                tileTriangles[base + counter] = int(idx);
            }
        }
    }
}
