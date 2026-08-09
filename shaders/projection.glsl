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

struct WorldTriangle {
    vec4 p1;
    vec4 p2;
    vec4 p3;
    
    int materialIndex;
};

uniform int triangleCount;
uniform vec3 cameraPosition;
uniform vec3 cameraForward;
uniform vec3 cameraUp;

layout(std430, binding=0) buffer ProjectedTrianglesBuffer {
    ProjectedTriangle projectedTriangles[];
};

layout(std430, binding=1) buffer WorldTrianglesBuffer {
    WorldTriangle worldTriangles[];
};

vec2 project(vec3 point, vec3 cameraRight) {
    vec3 a = cameraForward;
    vec3 b = point - cameraPosition;
    vec3 c = b - a;
    
    float alSquared = a.x*a.x + a.y*a.y + a.z*a.z;
    float blSquared = b.x*b.x + b.y*b.y + b.z*b.z;
    float clSquared = c.x*c.x + c.y*c.y + c.z*c.z;
    
    float bl = sqrt(blSquared);
    
    float t = 2 * sqrt(alSquared) * bl;
    t *= 1 / (alSquared + blSquared - clSquared);
    
    vec3 d = b * 1 / length(b) * t;
    
    vec3 diff = d - a;
    vec3 normalizeDiff = normalize(diff);
    
    float r = length(diff);
    float cosAngle = dot(normalizeDiff, cameraUp);
    
    float y = -cosAngle * r;
    float x = dot(normalizeDiff, cameraRight) * r;
 
    return vec2(x, y);
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= triangleCount) return;

    WorldTriangle tri = worldTriangles[idx];
    vec3 cameraRight = normalize(cross(cameraForward, cameraUp));

    vec3 verts[3] = vec3[](tri.p1.xyz, tri.p2.xyz, tri.p3.xyz);
    vec2 screenVerts[3];
    float dists[3];

    screenVerts[0] = project(verts[0], cameraRight);
    dists[0] = distance(cameraPosition, verts[0]);
    
    vec2 minBounds = screenVerts[0];
    vec2 maxBounds = screenVerts[0];

    for (int i = 1; i < 3; i++) {
        screenVerts[i] = project(verts[i], cameraRight);
        dists[i] = distance(cameraPosition, verts[i]);
        minBounds.x = min(screenVerts[i].x, minBounds.x);
        minBounds.y = min(screenVerts[i].y, minBounds.y);
        maxBounds.x = max(screenVerts[i].x, maxBounds.x);
        maxBounds.y = max(screenVerts[i].y, maxBounds.y);
    }

    ProjectedTriangle outTri;
    outTri.p1 = screenVerts[0];
    outTri.p2 = screenVerts[1];
    outTri.p3 = screenVerts[2];
    outTri.min = minBounds;
    outTri.max = maxBounds;
    outTri.depths[0] = dists[0];
    outTri.depths[1] = dists[1];
    outTri.depths[2] = dists[2];
    outTri.materialIndex = tri.materialIndex;
    outTri.padding = 0.0;
    projectedTriangles[idx] = outTri;
}
