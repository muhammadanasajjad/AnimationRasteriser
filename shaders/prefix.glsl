#version 430 core

layout(std430, binding = 2) buffer TileCount {
    int tileCounts[];
};

layout(std430, binding = 3) buffer TileOffsets {
    int tileOffsets[];
};

uniform int tileCount;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    int sum = 0;
    for (int i = 0; i < tileCount; i++) {
        tileOffsets[i] = sum;
        sum += tileCounts[i];
    }
    tileOffsets[tileCount] = sum;
}
