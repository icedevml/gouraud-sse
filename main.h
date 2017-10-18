#pragma once

struct point {
    int x;
    int y;

    float red;
    float green;
    float blue;
};

extern "C" void fillTriangleSSE(int img_width, unsigned char *pixels, point &v1,
                                point &v2, point &v3);

void fillTriangle(int img_width, unsigned char *pixels, point &v1, point &v2, point &v3);
