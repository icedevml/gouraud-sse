#include <algorithm>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <unistd.h>
#include <vector>

#ifndef SSE_BENCH_ONLY
#include "SDL/SDL.h"
#endif

#include "main.h"

bool comparator(const point &a, const point &b) {
    return a.y == b.y ? a.x < b.x : a.y < b.y;
}

void hsv2rgb(float h, float s, float v, float &r, float &g, float &b) {
    // hsv2rgb conversion algorithm (originally written in JavaScript)
    // brought from: http://stackoverflow.com/a/8208967

    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    }

    r *= 255;
    g *= 255;
    b *= 255;
}

#ifndef SSE_BENCH_ONLY
void updateTitle(int fps) {
    std::stringstream ss;
    ss << "Gouraud Shading (FPS: " << fps << ")";
    SDL_WM_SetCaption(ss.str().c_str(), ss.str().c_str());
}

template <typename T>
inline T delta(const T &current_val, const T &target_val,
               const T &max_amplitude, bool &non_zero) {
    // used to calculate displacement between current and target value in one
    // tick
    if (target_val != current_val) {
        non_zero = true;
    }

    return std::max(std::min(target_val - current_val, max_amplitude),
                    -max_amplitude);
}
#endif

int main(int argc, char *argv[]) {
#ifndef SSE_BENCH_ONLY
    std::cout << "Gouraud Shading" << std::endl
              << std::endl;

    if (argc < 8) {
        std::cerr
            << "Usage: ./tria <mode> <seed> <width> <height> <v1colorhex> "
               "<v2colorhex> "
               "<v3colorhex>"
            << std::endl
            << "    mode: one of \"sse\", \"cpp\", \"benchmark\"" << std::endl
            << "    seed: an input for the RNG used to generate vertex "
               "positions, if seed is 0 then it will be taken from the system "
               "clock"
            << std::endl
            << std::endl
            << "Example: ./prog sse 0 800 600 FF0000 00FF00 0000FF"
            << std::endl;
        return 1;
    }

    unsigned int seed = atoi(argv[2]);
    int img_width = atoi(argv[3]);
    int img_height = atoi(argv[4]);
    bool benchmark_mode = false;
#else
    if (argc < 4) {
        std::cerr
            << "Usage: ./tria <seed> <width> <height>"
            << std::endl;
        return 1;
    }
    unsigned int seed = atoi(argv[1]);
    int img_width = atoi(argv[2]);
    int img_height = atoi(argv[3]);
    bool benchmark_mode = true;
#endif

    if (!seed) {
        seed = (getpid() << 16) | (time(NULL) & 0x0000FFFF);
        std::cout << "Generated seed: " << seed << std::endl;
    }

    if (img_width < 100 && img_height < 100) {
        std::cerr << "Width and height parameters must be at least 100."
                  << std::endl;
        return 1;
    }

    auto fillTriangleImpl = fillTriangleSSE;

#ifndef SSE_BENCH_ONLY
    if (strcmp(argv[1], "cpp") == 0) {
        std::cout << "Using C++ implementation." << std::endl;
        fillTriangleImpl = fillTriangle;
    } else if (strcmp(argv[1], "sse") == 0) {
        std::cout << "Using NASM SSE implementation." << std::endl;
    } else if (strcmp(argv[1], "benchmark") == 0) {
        std::cout << "Performing a benchmark..." << std::endl;
        benchmark_mode = true;
    } else {
        std::cerr << "Unknown mode argument provided." << std::endl;
        return 1;
    }
#endif

    srand(seed);

    std::vector<point> vtx;  // current state
    std::vector<point> dvtx; // target state for animation
    std::vector<point> avtx; // current frame state

    while (true) {
        // repeat generation of vertices until we get some nice area between
        vtx.push_back({rand() % img_width, rand() % img_height});
        vtx.push_back({rand() % img_width, rand() % img_height});
        vtx.push_back({rand() % img_width, rand() % img_height});
        std::sort(vtx.begin(), vtx.end(), comparator);

        int area = abs((vtx[0].x * (vtx[1].y - vtx[2].y) +
                        vtx[1].x * (vtx[2].y - vtx[0].y) +
                        vtx[2].x * (vtx[0].y - vtx[1].y)) /
                       2);

        if (area >= img_width * img_height * 0.05) {
#ifndef SSE_BENCH_ONLY
            std::cout << "Generated a triangle with area: " << area
                      << std::endl;
#endif
            break;
        }

        vtx.clear();
    }

    int channels = 3; // for a RGB image

#ifndef SSE_BENCH_ONLY
    unsigned int v1color = std::stoul(std::string(argv[5]), nullptr, 16);
    unsigned int v2color = std::stoul(std::string(argv[6]), nullptr, 16);
    unsigned int v3color = std::stoul(std::string(argv[7]), nullptr, 16);
#else
    unsigned int v1color = 0xFF0000;
    unsigned int v2color = 0x00FF00;
    unsigned int v3color = 0x0000FF;
#endif

    vtx[0].red = v1color & 0xFF;
    vtx[0].green = (v1color >> 8) & 0xFF;
    vtx[0].blue = (v1color >> 16) & 0xFF;

    vtx[1].red = v2color & 0xFF;
    vtx[1].green = (v2color >> 8) & 0xFF;
    vtx[1].blue = (v2color >> 16) & 0xFF;

    vtx[2].red = v3color & 0xFF;
    vtx[2].green = (v3color >> 8) & 0xFF;
    vtx[2].blue = (v3color >> 16) & 0xFF;

    if (benchmark_mode) {
        int iterations = 100000;

        // pseudo surface
        unsigned char *pixels =
            new unsigned char[img_width * img_height * channels]();

        double start, end;

#ifndef SSE_BENCH_ONLY
        std::cout << std::endl
                  << "Benchmarking C++ implementation (" << iterations
                  << " iterations)..." << std::endl;
        start = 0;

        for (int i = 0; i < iterations+100; i++) {
            if (i == 100) {
                start = omp_get_wtime();
            }

            fillTriangle(img_width, pixels, vtx[0], vtx[1], vtx[2]);
        }

        end = omp_get_wtime();
        double cpp_us = (end - start) * 1000 * 1000 / iterations;

        std::cout << "Took time: " << cpp_us << " us" << std::endl << std::endl;

        std::cout << "Benchmarking SSE implementation (" << iterations
                  << " iterations)..." << std::endl;
#endif

        for (int i = 0; i < iterations+100; i++) {
            if (i == 100) {
                start = omp_get_wtime();
            }

            fillTriangleSSE(img_width, pixels, vtx[0], vtx[1], vtx[2]);
        }

        end = omp_get_wtime();
        double sse_us = (end - start) * 1000 * 1000 / iterations;

#ifndef SSE_BENCH_ONLY
        std::cout << "Took time: " << sse_us << " us" << std::endl;
        std::cout << "The second implementation is " << cpp_us / sse_us
                  << " times faster." << std::endl
                  << std::endl;
#else
        std::cout << sse_us << std::endl;
#endif

        delete[] pixels;
        return 0;
    }

#ifndef SSE_BENCH_ONLY
    dvtx = vtx;

    std::cout << "Generated vertices:" << std::endl;

    for (int i = 0; i <= 2; i++) {
        std::cout << "v" << i << ": (" << std::setw(3) << vtx[i].x << ", "
                  << std::setw(3) << vtx[i].y << ") (R:" << std::setfill(' ')
                  << std::setw(3) << vtx[i].red << ", G:" << std::setw(3)
                  << vtx[i].green << ", B:" << std::setw(3) << vtx[i].blue
                  << ")" << std::endl;
    }

    std::cout << std::endl;

    SDL_Surface *screen;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
        return 2;
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(img_width, img_height, 24,
                              SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ASYNCBLIT);
    if (screen == NULL) {
        std::cerr << "Couldn't set video mode: " << SDL_GetError() << std::endl;
        return 2;
    }

    updateTitle(0);

    SDL_EnableKeyRepeat(500, 500);
    SDL_Event event;

    std::cout << "Keymap: Press 1, 2 or 3 on keyboard to select vertex. Then "
                 "click somewhere on the screen in order to change the "
                 "position of vertex or press C to change it's color."
              << std::endl
              << std::endl;

    int currentV = 0;
    int lastTicks = SDL_GetTicks();
    int countedFrames = 0;
    bool need_recalculate = true;

    while (true) {
        // write zeros
        memset(screen->pixels, 0, img_width * img_height * channels);

        if (need_recalculate) {
            need_recalculate = false;

            for (int i = 0; i <= 2; i++) {
                vtx[i].x += delta(vtx[i].x, dvtx[i].x, 2, need_recalculate);
                vtx[i].y += delta(vtx[i].y, dvtx[i].y, 2, need_recalculate);

                vtx[i].red +=
                    delta(vtx[i].red, dvtx[i].red, 15.0f, need_recalculate);
                vtx[i].green +=
                    delta(vtx[i].green, dvtx[i].green, 15.0f, need_recalculate);
                vtx[i].blue +=
                    delta(vtx[i].blue, dvtx[i].blue, 15.0f, need_recalculate);
            }

            avtx = vtx;
            std::sort(avtx.begin(), avtx.end(), comparator);

            // prevent placing two vertices at exactly
            // the same y index not to cause numeric errors
            if (avtx[0].y == avtx[1].y) {
                avtx[1].y++;
            }

            if (avtx[1].y == avtx[2].y) {
                avtx[2].y++;
            }
        }

        fillTriangleImpl(img_width, (unsigned char *)screen->pixels, avtx[0],
                         avtx[1], avtx[2]);

        SDL_Flip(screen);

        countedFrames++;

        if (countedFrames == 100) {
            int ticks = SDL_GetTicks();
            updateTitle((int)(1000.0f / ((float)(ticks - lastTicks) / 100.0f)));

            countedFrames = 0;
            lastTicks = ticks;
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN: {
                bool vtx_select = false;

                if (event.key.keysym.sym == SDLK_x) {
                    exit(0);
                } else if (event.key.keysym.sym == SDLK_1) {
                    currentV = 0;
                    vtx_select = true;
                } else if (event.key.keysym.sym == SDLK_2) {
                    currentV = 1;
                    vtx_select = true;
                } else if (event.key.keysym.sym == SDLK_3) {
                    currentV = 2;
                    vtx_select = true;
                } else if (event.key.keysym.sym == SDLK_c) {
                    // generate new colors in HSV - this way we'll get very
                    // intense ones
                    float h = (float)(rand() % 1000) / 1000;
                    float s = (float)((rand() % 100) + 900.0f) / 1000;
                    float v = (float)((rand() % 100) + 900.0f) / 1000;

                    // convert to RGB
                    hsv2rgb(h, s, v, dvtx[currentV].red, dvtx[currentV].green,
                            dvtx[currentV].blue);

                    need_recalculate = true;

                    // report change
                    std::cout << "* Change vtx[" << currentV
                              << "] color to (R: " << dvtx[currentV].red
                              << ", G: " << dvtx[currentV].green
                              << ", B: " << dvtx[currentV].blue << ")"
                              << std::endl;
                }

                if (vtx_select) {
                    std::cout << "* Selected vtx[" << currentV << "] at ("
                              << vtx[currentV].x << ", " << vtx[currentV].y
                              << ")" << std::endl;
                }
            } break;

            case SDL_MOUSEBUTTONDOWN: {
                int x;
                int y;
                SDL_GetMouseState(&x, &y);

                dvtx[currentV].x = x;
                dvtx[currentV].y = y;
                need_recalculate = true;

                std::cout << "* Change vtx[" << currentV << "] position to ("
                          << x << ", " << y << ")" << std::endl;
            } break;

            case SDL_QUIT: {
                exit(0);
            } break;
            }
        }
    }

    std::cerr << "SDL_WaitEvent error: " << SDL_GetError() << std::endl;
#endif
    exit(0);
}
