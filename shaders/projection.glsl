#version 430 core

struct ProjectedTriangle {
    vec2 p1;
    vec2 p2;
    vec2 p3;
};

struct WorldTriangle {
    vec4 p1;
    vec4 p2;
    vec4 p3;
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

float PI = 3.14159265;

vec2 project(vec3 point) {
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
    vec3 cameraRight = normalize(cross(cameraForward, cameraUp));
    float x = dot(normalizeDiff, cameraRight) * r;
    
    return vec2(x, y);
}

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= triangleCount) return;

    WorldTriangle tri = worldTriangles[idx];
    vec3 right = normalize(cross(cameraForward, cameraUp));
    float fovScale = 1.0;

    vec3 verts[3] = vec3[](tri.p1.xyz, tri.p2.xyz, tri.p3.xyz);
    vec2 screenVerts[3];

    for (int i = 0; i < 3; i++) {
        vec3 point = verts[i];
        
        screenVerts[i] = project(verts[i]);
    }

    projectedTriangles[idx] = ProjectedTriangle(screenVerts[0], screenVerts[1], screenVerts[2]);
}
