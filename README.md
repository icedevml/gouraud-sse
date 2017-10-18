Gouraud Shading
===============

This program generates a random triangle and renders it using Gouraud shading method for 2D triangles. The result is displayed using SDL library. Two implementations are available: the standard C++ one (optimized by GCC) together with my standard implementation in NASM. There is an interactive mode, which allows to see some simple animations of vertex positions and colors.

Build with `make tria` command. The compiler must be compatible with C++11 standard. SDL 1.2 is required (x64 build). Supported OS: Ubuntu 16.04 LTS x86-64.

Dependencies
------------

For Ubuntu 16.04 LTS:
```
apt-get install build-essential libsdl1.2-dev nasm
```

Usage
-----

```
$ ./tria <mode> <seed> <width> <height> <v1colorhex> <v2colorhex> <v3colorhex>
```

* `mode` should be one of: `benchmark` (compare two implementations), `sse` (interactive mode using NASM SSE implementation), `cpp` (interactive mode using C++ implementation)
* `seed` is a seed for RNG which is used to generate the positions of vertices, the seed will be generated randomly if this argument is zero
* `width` and `height` are the window dimensions which must be at least `100`
* `v1colorhex`, `v2colorhex`, `v3colorhex` - colors of the particular vertices in RGB hex, e.g. `FF0000`

Exemplary usage
---------------

Compare two implementations:
```
$ ./tria benchmark 1337 800 600 FF0000 00FF00 0000FF
```

Display interactive NASM SSE version:
```
$ ./tria sse 0 800 600 FF0000 00FF00 0000FF
```

Display interactive C++ version:
```
$ ./tria cpp 0 800 600 FF0000 00FF00 0000FF
```

Interactive mode controls
-------------------------

Press `1`, `2` and `3` on the keyboard in order to select the vertex to be manipulated. After that, click somewhere on the screen to see the animation. You could also use `C` in order to generate a new random color for the selected vertex.

