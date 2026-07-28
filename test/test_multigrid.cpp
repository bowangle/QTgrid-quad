#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>
#include "type_double_double.h"
#include "grid.h"
#include "type_int128.h"
#include "type_float128_boost.h"
#include "grid_alias.h"

// --- convenience MultQTGrid aliases (matching QTGrid aliases in grid_alias.h) ---
template <typename Scalar, typename Sint>
using MultGrid = MultQTGrid<Scalar, Sint>;

using MultGridD_LL = MultQTGrid<double, long long>;
using MultGridD_QI = MultQTGrid<double, util::i128>;
using MultGridQ_LL = MultQTGrid<float128, long long>;
using MultGridQ_QI = MultQTGrid<float128, util::i128>;
using MultGridDD_LL = MultQTGrid<dd_128, long long>;
using MultGridDD_QI = MultQTGrid<dd_128, util::i128>;


template <typename Grid>
void test_one_multigrid_roundtrip(const char* name, std::vector<int> nBits) {
    int dim = static_cast<int>(nBits.size());
    std::cout << "[TEST] " << name << " dim=" << dim << " nBits=[";
    for (size_t i = 0; i < nBits.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << nBits[i];
    }
    std::cout << "]\n";

    using Scalar = typename Grid::scalar_type;

    std::vector<Scalar> a(dim, Scalar(-5));
    std::vector<Scalar> b(dim, Scalar(10));
    Grid g(a, b, nBits);

    std::vector<Scalar> x(dim, Scalar(1.25));

    std::cout << std::scientific << std::setprecision(std::numeric_limits<Scalar>::max_digits10);
    std::cout << "a=[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << g.get_a()[i]; }
    std::cout << "]\n";
    std::cout << "b=[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << g.get_b()[i]; }
    std::cout << "]\n";
    std::cout << "N=[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << g.get_N()[i]; }
    std::cout << "]\n";
    std::cout << "tensorLen=" << g.get_tensorLen() << "\n";

    auto bits = g.coord_to_id(x);
    auto x2 = g.id_to_coord(bits);

    auto bits_2 = g.coord_to_id(x2);
    auto x3 = g.id_to_coord(bits_2);

    std::cout << "x =[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << x[i]; }
    std::cout << "]\n";
    std::cout << "x2=[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << x2[i]; }
    std::cout << "]\n";
    std::cout << "x3=[";
    for (int i = 0; i < dim; ++i) { if (i > 0) std::cout << ","; std::cout << x3[i]; }
    std::cout << "]\n";

    if constexpr (std::is_same_v<Scalar, double>) {
        for (int i = 0; i < dim; ++i)
            assert(std::abs(double(x2[i] - x3[i])) < 1e-6);
    }
}


void test_multigrid_roundtrip() {
    // dim=1 (should behave like QTGrid)
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {8});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {8});
    test_one_multigrid_roundtrip<MultGridQ_LL>("MultGridQ_LL", {8});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {8});
    test_one_multigrid_roundtrip<MultGridDD_LL>("MultGridDD_LL", {8});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {8});

    // dim=2, equal bits
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {8, 8});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {8, 8});
    test_one_multigrid_roundtrip<MultGridQ_LL>("MultGridQ_LL", {8, 8});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {8, 8});
    test_one_multigrid_roundtrip<MultGridDD_LL>("MultGridDD_LL", {8, 8});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {8, 8});

    // dim=3, equal bits
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {8, 8, 8});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {8, 8, 8});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {8, 8, 8});

    // dim=2, unequal bits
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {10, 6});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {10, 6});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {10, 6});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {10, 6});

    // dim=3, unequal bits
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {12, 8, 4});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {12, 8, 4});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {12, 8, 4});

    // large nBits (still < 64, safe for double)
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {30});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {30});
    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {30, 30});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {30, 30});

    test_one_multigrid_roundtrip<MultGridD_LL>("MultGridD_LL", {48});
    test_one_multigrid_roundtrip<MultGridD_QI>("MultGridD_QI", {48});
    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {48});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {48});

    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {63});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {63});

    test_one_multigrid_roundtrip<MultGridQ_QI>("MultGridQ_QI", {100});
    test_one_multigrid_roundtrip<MultGridDD_QI>("MultGridDD_QI", {100});
}


template <typename Scalar, typename Sint>
void test_multigrid() {
    std::vector<Scalar> a = {Scalar(-5.), Scalar(0.)};
    std::vector<Scalar> b = {Scalar(10.), Scalar(1.)};
    std::vector<int> nBits = {15, 10};

    MultQTGrid<Scalar, Sint> grid(a, b, nBits);

    std::cout << "[test_multigrid] dim=" << grid.get_dim()
              << " tensorLen=" << grid.get_tensorLen()
              << " deltaVolume=" << grid.delta_volume() << "\n";

    assert(grid.get_dim() == 2);
    assert(grid.get_tensorLen() == 25);
    assert(grid.get_N()[0] == (Sint(1) << 15));
    assert(grid.get_N()[1] == (Sint(1) << 10));
}


template <typename Scalar, typename Sint>
void test_save_load_roundtrip_multigrid() {
    std::vector<Scalar> a = {Scalar(-5.0), Scalar(0.0), Scalar(2.0)};
    std::vector<Scalar> b = {Scalar(10.0), Scalar(1.0), Scalar(8.0)};
    std::vector<int> nBits = {15, 10, 8};

    MultQTGrid<Scalar, Sint> grid(a, b, nBits);

    // ---------------- SAVE ----------------
    std::string filename = "multigrid_test";
    grid.save_json(filename);

    // ---------------- LOAD (exact) ----------------
    MultQTGrid<Scalar, Sint> grid2(filename + "_multgrid_E.json");

    // ---------------- CHECKS ----------------

    auto almost_equal = [](Scalar x, Scalar y) {
        using std::abs;
        using boost::multiprecision::abs;
        return abs(x - y) < Scalar(1e-12);
    };

    assert(grid.get_dim() == grid2.get_dim());
    std::cout << "dim: " << grid.get_dim() << " vs " << grid2.get_dim() << "\n";

    for (int i = 0; i < grid.get_dim(); ++i) {
        std::cout << "a[" << i << "]: " << grid.get_a()[i] << " vs " << grid2.get_a()[i] << "\n";
        std::cout << "b[" << i << "]: " << grid.get_b()[i] << " vs " << grid2.get_b()[i] << "\n";
        std::cout << "nBits[" << i << "]: " << grid.get_nBits()[i] << " vs " << grid2.get_nBits()[i] << "\n";
        std::cout << "N[" << i << "]: " << grid.get_N()[i] << " vs " << grid2.get_N()[i] << "\n";

        assert(almost_equal(grid.get_a()[i], grid2.get_a()[i]));
        assert(almost_equal(grid.get_b()[i], grid2.get_b()[i]));
        assert(grid.get_nBits()[i] == grid2.get_nBits()[i]);
        assert(grid2.get_N()[i] == (Sint(1) << grid2.get_nBits()[i]));

        Scalar dx1 = (grid.get_b()[i] - grid.get_a()[i]) / Scalar(grid.get_N()[i]);
        Scalar dx2 = (grid2.get_b()[i] - grid2.get_a()[i]) / Scalar(grid2.get_N()[i]);
        assert(almost_equal(dx1, dx2));
    }

    // roundtrip a point through both grids
    std::vector<Scalar> x = {Scalar(1.25), Scalar(0.5), Scalar(5.5)};
    auto bits1 = grid.coord_to_id(x);
    auto bits2 = grid2.coord_to_id(x);

    assert(static_cast<int>(bits1.size()) == grid.get_tensorLen());
    assert(static_cast<int>(bits2.size()) == grid2.get_tensorLen());
    for (size_t i = 0; i < bits1.size(); ++i)
        assert(bits1[i] == bits2[i]);

    auto x1r = grid.id_to_coord(bits1);
    auto x2r = grid2.id_to_coord(bits2);
    for (int i = 0; i < grid.get_dim(); ++i)
        assert(almost_equal(x1r[i], x2r[i]));

    std::cout << "MultQTGrid save/load roundtrip test PASSED\n";
}


int main() {

    test_multigrid<double, long long>();
    test_multigrid<double, util::i128>();
    test_multigrid<float128, long long>();
    test_multigrid<float128, util::i128>();
    test_multigrid<dd_128, long long>();
    test_multigrid<dd_128, util::i128>();

    test_save_load_roundtrip_multigrid<double, long long>();
    test_save_load_roundtrip_multigrid<double, util::i128>();
    test_save_load_roundtrip_multigrid<float128, long long>();
    test_save_load_roundtrip_multigrid<float128, util::i128>();
    test_save_load_roundtrip_multigrid<dd_128, long long>();
    test_save_load_roundtrip_multigrid<dd_128, util::i128>();

    test_multigrid_roundtrip();

    std::cout << "\n=== All MultQTGrid tests PASSED ===\n";
    return 0;
}
