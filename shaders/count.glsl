#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
    
    float depths[3];

    int materialIndex;
};

layout(std430, binding = 0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

uniform int projectedTriangleCount;
uniform int tileRows;
uniform int tileColumns;

layout(std430, binding = 2) buffer TileCount {
    int tileCounts[];
};

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= projectedTriangleCount)
        return;

    ProjectedTriangle tri = projectedTriangles[idx];

    vec2 triMin = min(tri.p1, min(tri.p2, tri.p3));
    vec2 triMax = max(tri.p1, max(tri.p2, tri.p3));

    ivec2 minTile = ivec2(max(0, int(ceil(((triMin.x + 1.0) / 2.0) * tileColumns - 1.0))),
                          max(0, int(ceil(((1.0 - triMax.y) / 2.0) * tileRows - 1.0))));
    ivec2 maxTile = ivec2(min(tileColumns - 1, int(floor(((triMax.x + 1.0) / 2.0) * tileColumns))),
                          min(tileRows - 1, int(floor(((1.0 - triMin.y) / 2.0) * tileRows))));

    for (int ty = minTile.y; ty <= maxTile.y; ty++) {
        for (int tx = minTile.x; tx <= maxTile.x; tx++) {
            atomicAdd(tileCounts[ty * tileColumns + tx], 1);
        }
    }
}
