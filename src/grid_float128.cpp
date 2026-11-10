#include <QTgrid-quad/grid.h>

template class QTGrid<float128, int>;
template class QTGrid<float128, long>;
template class QTGrid<float128, long long>;
template class QTGrid<float128, util::i128>;

template class MultQTGrid<float128, int>;
template class MultQTGrid<float128, long>;
template class MultQTGrid<float128, long long>;
template class MultQTGrid<float128, util::i128>;
