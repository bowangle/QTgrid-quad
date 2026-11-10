#pragma once

#include <numeric-type-quad/type_double_double.h>
#include <numeric-type-quad/type_float128_boost.h>
#include <numeric-type-quad/type_int128.h>
#include <QTgrid-quad/grid.h>

using GridD_LL = QTGrid<double, long long>;
using GridD_QI = QTGrid<double, util::i128>;

using GridQ_LL = QTGrid<float128, long long>;
using GridQ_QI   = QTGrid<float128, util::i128>;

using GridDD_LL = QTGrid<dd_128, long long>;
using GridDD_QI   = QTGrid<dd_128, util::i128>;
