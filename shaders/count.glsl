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

layout(std430, binding = 2) buffer TileCount {
    int tileCounts[];
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() {
    float x = float(gl_GlobalInvocationID.x);
    float y = float(gl_GlobalInvocationID.y);

    if (x >= tileRows || y >= tileColumns)
        return;

    int count = 0;
    
    vec2 tileSize = vec2(1.0 / float(tileColumns), 1.0 / float(tileRows));

    vec2 tileMin = vec2(x, y) * tileSize;
    vec2 tileMax = tileMin + tileSize;
    tileMin.x = tileMin.x * 2.0 - 1.0;
    tileMin.y = 1.0 - tileMin.y * 2.0;
    tileMax.x = tileMax.x * 2.0 - 1.0;
    tileMax.y = 1.0 - tileMax.y * 2.0;

    for (int i = 0; i < projectedTriangleCount; i++) {
        ProjectedTriangle triangle = projectedTriangles[i];

        float hit =
            step(tileMin.x, triangle.max.x) *
            step(triangle.min.x, tileMax.x) *
            step(tileMax.y, triangle.max.y) *
            step(triangle.min.y, tileMin.y);

        count += int(round(hit));
    }

    tileCounts[int(y) * tileColumns + int(x)] = count;
}
