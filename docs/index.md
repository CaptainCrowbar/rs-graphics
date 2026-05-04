# Graphics Library

By Ross Smith

_[GitHub repository](https://github.com/CaptainCrowbar/rs-graphics)_

## Overview

```c++
namespace RS::Graphics;
```

My library of basic graphics classes, highly generalized and templated.

## Index

* [`rs-graphics/colour.hpp` -- Colour](colour.html)
* [`rs-graphics/colour-space.hpp` -- Colour space](colour-space.html)
* [`rs-graphics/font.hpp` -- Font](geometry.html)
* [`rs-graphics/geometry.hpp` -- Geometric primitives](geometry.html)
* [`rs-graphics/image.hpp` -- Image](image.html)
* [`rs-graphics/noise.hpp` -- Pseudo-random noise](noise.html)
* [`rs-graphics/projection.hpp` -- Map projections](projection.html)
* [`rs-graphics/version.hpp` -- Version information](version.html)

## Using the library

You will need my header-only
[core utility library](https://github.com/CaptainCrowbar/rs-core).

There is a `CMakeLists.txt` file that can build and install the library using
the usual [CMake](https://cmake.org) conventions. Command line usage will
typically look like this:

```bash
cd wherever/you/installed/rs-graphics
mkdir build
cd build
cmake -G "Unix Makefiles" ../src
    # or cmake -G "Visual Studio 17 2022" ../src on Windows
cmake --build . --config Release
cmake --build . --config Release --target install'
```

The library's public headers are listed above (other headers are for internal
use only and should not be included by your code). To use the library,
`#include` the individual headers you want. Link your build with
`-lrs-graphics.`
