#include "main.h"

template<typename T>
inline void swap(T& t1, T& t2) {
    T temp = t1;
    t1 = t2;
    t2 = temp;
}

void fillTriangle(int img_width, unsigned char *pixels, point &v1, point &v2,
                  point &v3) {
    float d0 = (float)(v2.x - v1.x) / (v2.y - v1.y);
    float d1 = (float)(v3.x - v1.x) / (v3.y - v1.y);

    int y = v1.y;

    float curx1 = v1.x;
    float curx2 = v1.x;

    int row_height = img_width * 3;
    unsigned char *row = pixels + y * row_height;

    while (true) {
        float coeffIa1 = (float)(y - v3.y) / (v1.y - v3.y);
        float coeffIa2 = (float)(v1.y - y) / (v1.y - v3.y);

        float coeffIb1;
        float coeffIb2;

        if (y <= v2.y) {
            coeffIb1 = (float)(y - v2.y) / (v1.y - v2.y);
            coeffIb2 = (float)(v1.y - y) / (v1.y - v2.y);
        } else {
            coeffIb1 = (float)(y - v3.y) / (v2.y - v3.y);
            coeffIb2 = (float)(v2.y - y) / (v2.y - v3.y);
        }

        float Iar = coeffIa1 * v1.red + coeffIa2 * v3.red;
        float Iag = coeffIa1 * v1.green + coeffIa2 * v3.green;
        float Iab = coeffIa1 * v1.blue + coeffIa2 * v3.blue;

        float Ibr;
        float Ibg;
        float Ibb;

        if (y <= v2.y) {
            Ibr = coeffIb1 * v1.red + coeffIb2 * v2.red;
            Ibg = coeffIb1 * v1.green + coeffIb2 * v2.green;
            Ibb = coeffIb1 * v1.blue + coeffIb2 * v2.blue;
        } else {
            Ibr = coeffIb1 * v2.red + coeffIb2 * v3.red;
            Ibg = coeffIb1 * v2.green + coeffIb2 * v3.green;
            Ibb = coeffIb1 * v2.blue + coeffIb2 * v3.blue;
        }

        int x1 = curx1;
        int x2 = curx2;

        if (x1 > x2) {
            swap(x1, x2);

            swap(Ibr, Iar);
            swap(Ibg, Iag);
            swap(Ibb, Iab);
        }

        for (int x = x1; x <= x2; x++) {
            float coeff1 = (float)(x2 - x) / (x2 - x1);
            float coeff2 = (float)(x - x1) / (x2 - x1);

            float intensity_r = coeff1 * Ibr + coeff2 * Iar;
            float intensity_g = coeff1 * Ibg + coeff2 * Iag;
            float intensity_b = coeff1 * Ibb + coeff2 * Iab;

            int x_offset = x * 3;

            row[x_offset] = (unsigned char)intensity_b;
            row[x_offset + 1] = (unsigned char)intensity_g;
            row[x_offset + 2] = (unsigned char)intensity_r;
        }

        if (y == v3.y) {
            break;
        } else if (y == v2.y) {
            d0 = (float)(v3.x - v2.x) / (v3.y - v2.y);
        }

        y++;
        row += row_height;
        curx1 += d0;
        curx2 += d1;
    }

    // draw a green rectangle in left-upper corner
    // which will allow to distinguish this implementation
    for (int my = 0; my < 5; my++) {
        for (int mx = 0; mx < 15; mx += 3) {
            pixels[my * row_height + mx] = 0;
            pixels[my * row_height + mx + 1] = 255;
            pixels[my * row_height + mx + 2] = 0;
        }
    }
}

