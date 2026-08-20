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

struct Material {
    vec4 colour;
    int textureIndex;
};

uniform int projectedTriangleCount;
uniform int tileRows;
uniform int tileColumns;
uniform vec3 lightDir;
uniform int textureCount;

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

#define MAX_TEXTURES 64
uniform sampler2D textures[MAX_TEXTURES];

float ANTIALIASING_SCALE = 0.0000001;

in vec2 textureCoords;

out vec4 FragColor;

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

    vec4 colour = vec4(0.0);
    float currentDepth = 1e4;

    for (int i = start; i < end; i++) {
        int triIdx = tileTriangles[i];
        ProjectedTriangle tri = projectedTriangles[triIdx];
        Material material = materials[tri.materialIndex];

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

            float q1 = w1 / tri.depths[0];
            float q2 = w2 / tri.depths[1];
            float q3 = w3 / tri.depths[2];
            float qSum = q1 + q2 + q3;

            vec2 interpUV = (q1 * tri.uv1 + q2 * tri.uv2 + q3 * tri.uv3) / qSum;
            vec3 interpNormal = normalize(
                (q1 * tri.n1.xyz + q2 * tri.n2.xyz + q3 * tri.n3.xyz) / qSum);

            float NdotL = max(dot(interpNormal, lightDir), 0.0);
            float ambient = 0.15;
            float lighting = ambient + (1.0 - ambient) * NdotL;

            vec4 baseColour;
            if (material.textureIndex >= 0 && material.textureIndex < textureCount) {
                baseColour = texture(textures[material.textureIndex], interpUV) * material.colour;
            } else {
                baseColour = material.colour;
            }

            colour = vec4(baseColour.rgb * lighting, baseColour.a) * inside;
        }
    }

    FragColor = colour;
}
