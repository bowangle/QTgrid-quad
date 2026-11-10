# QTgrid-quad

Standalone C++20 library for `QTGrid` and `MultQTGrid`. The default build produces
`libqtgrid_quad.a`. All handwritten CMake configuration is in `CMakeLists.txt`;
package files are generated automatically in the build directory.

## Dependencies

- An already installed `numeric-type-quad` package, normally under the repository's
  `install/` prefix. It supplies Eigen, QD, system Boost, and quadmath.
- JSON headers from the shared `extern/json/include/` directory.

This layer never downloads or rebuilds lower layers. If numeric is built but not
installed yet, install the existing binary once from the repository root:

```sh
cmake --install numeric-type-quad/build --prefix "$PWD/install"
```

## Build, install, test

From the repository root:

```sh
# Configure and compile only.
bash TN-library/QTgrid-quad/compile.sh

# Configure, compile, and install into the repository's local install/ directory.
bash TN-library/QTgrid-quad/install.sh

# Run compiled tests without rebuilding; print and save their full output.
bash TN-library/QTgrid-quad/run-test.sh
```

Scripts work from any working directory and preserve the build cache. The build
is in `TN-library/QTgrid-quad/build/`, and the test log is `build/test-output.txt`
inside that project. Each test run replaces the log; failures return a nonzero
exit status. JSON roundtrip files are isolated under `build/test-output/`.
The build script uses the original Release flags `-O3 -march=native` rather
than CMake's default `-O3 -DNDEBUG`. Tests additionally use
`-Wall -Wextra -Wreorder -Wshadow -UNDEBUG`, keeping assertions active.
Library objects use `-fPIC` so they can be linked into shared libraries or Python
extensions. `-march=native` targets the build machine's CPU, so these binaries
may not run on machines with older CPUs.

`BUILD_DIR` overrides the build directory, `BUILD_JOBS` controls parallelism,
and `INSTALL_PREFIX` overrides the install script's destination. No `sudo` is
needed for the default local install.

Additional compile/install arguments are forwarded to CMake, for example:

```sh
bash TN-library/QTgrid-quad/install.sh \
  -DCMAKE_PREFIX_PATH=/path/to/numeric/install \
  -DCMAKE_INSTALL_PREFIX=/path/to/qtgrid/install
```

Other options: `-DQTGRID_BUILD_TESTS=OFF`, `-DBUILD_SHARED_LIBS=ON`, and
`-DQTGRID_EXTERN_DIR=/path/to/extern`. The test script forwards arguments to CTest,
for example `-R qtgrid_library` to select one test.

## Consume from the next layer

```cmake
find_package(QTgrid-quad CONFIG REQUIRED)
target_link_libraries(my_library PUBLIC qtgrid_quad::qtgrid_quad)
```

```cpp
#include <QTgrid-quad/grid.h>
#include <QTgrid-quad/grid_alias.h>
```

Public headers are under `include/QTgrid-quad/` both in the source tree and in
the install prefix. Configure consumers with `-DCMAKE_PREFIX_PATH=/path/to/install`;
include both prefixes if numeric and QTgrid are installed separately. The numeric
package is found transitively without rebuilding it.

The installed target records the JSON header path. If JSON moves, reconfigure
QTgrid with the new `QTGRID_EXTERN_DIR` and reinstall it. Dependencies must remain
available; the installation is not a bundled distribution.

## Explicit instantiations

The three `src/grid_*.cpp` files instantiate both grid classes for every
combination of scalar (`double`, `dd_128`, `float128`) and index (`int`, `long`,
`long long`, `util::i128`). Both `long` and `long long` are included because
`int64_t` commonly aliases `long` on Linux.

Matching `extern template` declarations use the compiled specializations.
Template definitions remain available in the header for other type combinations.
