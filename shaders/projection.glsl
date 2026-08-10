#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
    
    float depths[3];

    int materialIndex;
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

void clipTriangle(vec3 v0, vec3 v1, vec3 v2, out vec3 c0, out vec3 c1, out vec3 c2, out vec3 c3, out int count) {
    float nearPlane = 0.1;

    vec3 verts[3] = vec3[](v0, v1, v2);
    float dists[3];
    for (int i = 0; i < 3; i++) {
        dists[i] = dot(verts[i] - cameraPosition, cameraForward) - nearPlane;
    }

    vec3 outVerts[4] = vec3[](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
    int outCount = 0;
    for (int i = 0; i < 3; i++) {
        vec3 a = verts[i];
        vec3 b = verts[(i + 1) % 3];
        float da = dists[i];
        float db = dists[(i + 1) % 3];

        if (da >= 0.0) {
            outVerts[outCount] = a;
            outCount++;
        }
        if ((da >= 0.0) != (db >= 0.0)) {
            float t = da / (da - db);
            outVerts[outCount] = a + t * (b - a);
            outCount++;
        }
    }

    count = outCount;
    c0 = outVerts[0];
    c1 = outVerts[1];
    c2 = outVerts[2];
    c3 = outVerts[3];
}

void writeProjectedTriangle(vec2 p1, vec2 p2, vec2 p3, float d1, float d2, float d3, int material, uint slot) {
    ProjectedTriangle outTri;
    outTri.p1 = p1;
    outTri.p2 = p2;
    outTri.p3 = p3;
    outTri.depths[0] = d1;
    outTri.depths[1] = d2;
    outTri.depths[2] = d3;
    outTri.materialIndex = material;
    projectedTriangles[slot] = outTri;
}

bool offScreen(vec2 minB, vec2 maxB) {
    return maxB.x < -1.0 || minB.x > 1.0 || maxB.y < -1.0 || minB.y > 1.0;
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= triangleCount) return;

    WorldTriangle tri = worldTriangles[idx];
    vec3 cameraRight = normalize(cross(cameraForward, cameraUp));

    vec3 verts[3] = vec3[](tri.p1.xyz, tri.p2.xyz, tri.p3.xyz);

    vec3 cv0, cv1, cv2, cv3;
    int clipCount;
    clipTriangle(verts[0], verts[1], verts[2], cv0, cv1, cv2, cv3, clipCount);

    uint slot0 = idx * 2;
    uint slot1 = idx * 2 + 1;

    if (clipCount == 0) {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), 0.0, 0.0, 0.0, tri.materialIndex, slot0);
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
        return;
    }

    vec2 s0 = project(cv0, cameraRight);
    vec2 s1 = project(cv1, cameraRight);
    vec2 s2 = project(cv2, cameraRight);
    vec2 s3 = project(cv3, cameraRight);

    float dd0 = distance(cameraPosition, cv0);
    float dd1 = distance(cameraPosition, cv1);
    float dd2 = distance(cameraPosition, cv2);
    float dd3 = distance(cameraPosition, cv3);

    vec2 minB = min(s0, min(s1, s2));
    vec2 maxB = max(s0, max(s1, s2));
    if (offScreen(minB, maxB)) {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), 0.0, 0.0, 0.0, tri.materialIndex, slot0);
    } else {
        writeProjectedTriangle(s0, s1, s2, dd0, dd1, dd2, tri.materialIndex, slot0);
    }

    if (clipCount == 4) {
        vec2 minB2 = min(s0, min(s2, s3));
        vec2 maxB2 = max(s0, max(s2, s3));
        if (offScreen(minB2, maxB2)) {
            writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
        } else {
            writeProjectedTriangle(s0, s2, s3, dd0, dd2, dd3, tri.materialIndex, slot1);
        }
    } else {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
    }
}
