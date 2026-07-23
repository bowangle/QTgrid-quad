# QTgrid-quad

Header-only quad-precision grid library providing the `QTGrid<Scalar, Sint>` template class with coordinate-to-index mapping and JSON I/O. No compilation needed for use — just `#include "grid.h"`. The `test/` directory and `compile.sh` are only for running the unit tests.

## Quick start

```bash
# 1. Clone with submodules
git clone --recurse-submodules https://github.com/bowangle/QTgrid-quad.git
cd QTgrid-quad

# 2. Install dependencies (Eigen, QD, nlohmann/json)
bash install_extern.sh

# 3. Compile and run tests
bash compile.sh

# 4. Or run manually
./build/TestGrid
```

## Dependencies

Installed automatically by `install_extern.sh`:
- [numeric-type-quad](https://github.com/bowangle/numeric-type-quad) — double-double, float128, i128 types
- [Eigen](https://gitlab.com/libeigen/eigen) — linear algebra (header-only)
- [QD](https://github.com/BL-highprecision/QD.git) — double-double and quad-double arithmetic
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing (header-only)

## Supported types

`QTGrid<Scalar, Sint>` with:
- **Scalar**: `double`, `float128` (Boost), `dd_128` (double-double)
- **Sint**: `long long`, `util::i128`

Convenience aliases in `grid_alias.h`: `GridD_LL`, `GridQ_QI`, `GridDD_QI`, etc.
