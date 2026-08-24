#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
    vec2 uv1;
    vec2 uv2;
    vec2 uv3;
    vec4 n1;
    vec4 n2;
    vec4 n3;
    float depths[3];
    int materialIndex;
};

struct WorldTriangle {
    vec4 p1;
    vec4 p2;
    vec4 p3;
    vec4 n1;
    vec4 n2;
    vec4 n3;
    vec2 uv1;
    vec2 uv2;
    vec2 uv3;
    int materialIndex;
    float _pad;
};

uniform int triangleCount;
uniform vec3 cameraPosition;
uniform vec3 cameraForward;
uniform vec3 cameraUp;
uniform float aspectRatio;

const float nearPlane = 0.1;
const float clipEpsilon = 0.00001;
const float clipEpsilon2 = 0.01;

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

    return vec2(x / aspectRatio, y);
}

int clipTriangle(vec3 n, float d,
    inout vec3 v0, inout vec3 v1, inout vec3 v2, out vec3 v3,
    inout vec2 u0, inout vec2 u1, inout vec2 u2, out vec2 u3,
    inout vec3 nm0, inout vec3 nm1, inout vec3 nm2, out vec3 nm3) {

    vec3 dist = vec3(dot(v0, n) - d, dot(v1, n) - d, dot(v2, n) - d);

    if (!any(greaterThanEqual(dist, vec3(clipEpsilon2)))) {
        return 0;
    }

    if (all(greaterThanEqual(dist, vec3(-clipEpsilon)))) {
        v3 = v0; u3 = u0; nm3 = nm0;
        return 3;
    }

    bvec3 above = greaterThanEqual(dist, vec3(0.0));
    bool nextIsAbove;

    if (above[1] && !above[0]) {
        nextIsAbove = above[2];
        v3 = v0; v0 = v1; v1 = v2; v2 = v3;
        u3 = u0; u0 = u1; u1 = u2; u2 = u3;
        nm3 = nm0; nm0 = nm1; nm1 = nm2; nm2 = nm3;
        dist = dist.yzx;
    } else if (above[2] && !above[1]) {
        nextIsAbove = above[0];
        v3 = v2; v2 = v1; v1 = v0; v0 = v3;
        u3 = u2; u2 = u1; u1 = u0; u0 = u3;
        nm3 = nm2; nm2 = nm1; nm1 = nm0; nm0 = nm3;
        dist = dist.zxy;
    } else {
        nextIsAbove = above[1];
    }

    float t02 = dist[0] / (dist[0] - dist[2]);
    v3 = mix(v0, v2, t02);
    u3 = mix(u0, u2, t02);
    nm3 = mix(nm0, nm2, t02);

    if (nextIsAbove) {
        float t12 = dist[1] / (dist[1] - dist[2]);
        v2 = mix(v1, v2, t12);
        u2 = mix(u1, u2, t12);
        nm2 = mix(nm1, nm2, t12);
        return 4;
    } else {
        float t01 = dist[0] / (dist[0] - dist[1]);
        v1 = mix(v0, v1, t01);
        u1 = mix(u0, u1, t01);
        nm1 = mix(nm0, nm1, t01);
        v2 = v3;
        u2 = u3;
        nm2 = nm3;
        v3 = v0;
        u3 = u0;
        nm3 = nm0;
        return 3;
    }
}

void writeProjectedTriangle(vec2 p1, vec2 p2, vec2 p3,
    vec2 uv1, vec2 uv2, vec2 uv3,
    vec3 n1, vec3 n2, vec3 n3,
    float d1, float d2, float d3, int material, uint slot) {
    ProjectedTriangle outTri;
    outTri.p1 = p1;
    outTri.p2 = p2;
    outTri.p3 = p3;
    outTri.uv1 = uv1;
    outTri.uv2 = uv2;
    outTri.uv3 = uv3;
    outTri.n1 = vec4(n1, 0.0);
    outTri.n2 = vec4(n2, 0.0);
    outTri.n3 = vec4(n3, 0.0);
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

    vec3 v0 = tri.p1.xyz;
    vec3 v1 = tri.p2.xyz;
    vec3 v2 = tri.p3.xyz;
    vec3 v3;

    vec2 u0 = tri.uv1;
    vec2 u1 = tri.uv2;
    vec2 u2 = tri.uv3;
    vec2 u3;

    vec3 nm0 = tri.n1.xyz;
    vec3 nm1 = tri.n2.xyz;
    vec3 nm2 = tri.n3.xyz;
    vec3 nm3;

    vec3 n = cameraForward;
    float d = dot(cameraPosition, cameraForward) + nearPlane;

    int clipCount = clipTriangle(n, d, v0, v1, v2, v3, u0, u1, u2, u3, nm0, nm1, nm2, nm3);

    uint slot0 = idx * 2;
    uint slot1 = idx * 2 + 1;

    if (clipCount == 0) {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), vec2(0.0), vec2(0.0), vec2(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, tri.materialIndex, slot0);
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), vec2(0.0), vec2(0.0), vec2(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
        return;
    }

    vec3 cameraRight = normalize(cross(cameraForward, cameraUp));

    vec2 s0 = project(v0, cameraRight);
    vec2 s1 = project(v1, cameraRight);
    vec2 s2 = project(v2, cameraRight);

    float dd0 = dot(v0 - cameraPosition, cameraForward);
    float dd1 = dot(v1 - cameraPosition, cameraForward);
    float dd2 = dot(v2 - cameraPosition, cameraForward);

    vec2 minB = min(s0, min(s1, s2));
    vec2 maxB = max(s0, max(s1, s2));
    if (offScreen(minB, maxB)) {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), vec2(0.0), vec2(0.0), vec2(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, tri.materialIndex, slot0);
    } else {
        writeProjectedTriangle(s0, s1, s2, u0, u1, u2, nm0, nm1, nm2, dd0, dd1, dd2, tri.materialIndex, slot0);
    }

    if (clipCount == 4) {
        vec2 s3 = project(v3, cameraRight);
        float dd3 = dot(v3 - cameraPosition, cameraForward);

        vec2 minB2 = min(s0, min(s2, s3));
        vec2 maxB2 = max(s0, max(s2, s3));
        if (offScreen(minB2, maxB2)) {
            writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), vec2(0.0), vec2(0.0), vec2(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
        } else {
            writeProjectedTriangle(s0, s2, s3, u0, u2, u3, nm0, nm2, nm3, dd0, dd2, dd3, tri.materialIndex, slot1);
        }
    } else {
        writeProjectedTriangle(vec2(2.0), vec2(2.0), vec2(2.0), vec2(0.0), vec2(0.0), vec2(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, tri.materialIndex, slot1);
    }
}
