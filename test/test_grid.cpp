#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "type_double_double.h"
#include "grid.h"
#include "type_int128.h"
#include "type_float128_boost.h"
#include "grid_alias.h"

template <typename Grid>
void test_one_grid_roudtrip(const char* name, const int nBit) {
    std::cout << "[TEST] " << name << " nBit=" << nBit << "\n";

    using Scalar = typename Grid::scalar_type;

    Grid g(-5, 10, nBit);

    Scalar x = Scalar(1.25);

    std::cout << std::scientific << std::setprecision(std::numeric_limits<Scalar>::max_digits10) << "a=" << g.get_a() <<"\n";
    std::cout << "b="<< g.get_b() <<"\n";
    std::cout << "N="<< g.get_N() <<"\n";
    std::cout << "nBits="<< g.get_nBits() <<"\n";

    auto bits = g.coord_to_id(x);
    Scalar x2 = g.id_to_coord(bits);
    // x2 is x mapped on grid

    auto bits_2 = g.coord_to_id(x2);
    Scalar x3 = g.id_to_coord(bits_2);

    std::cout << std::scientific << std::setprecision(std::numeric_limits<Scalar>::max_digits10)
        <<"x = " << x <<"; x2 = " << x2 << " -> x3 = " << x3 << "\n";

    if constexpr (std::is_same_v<Scalar, double>) {
        assert(std::abs(double(x2 - x3)) < 1e-6);
    }
}

void test_grid_roundtrip(){
    test_one_grid_roudtrip<GridD_LL>("GridD_LL", 8);
    test_one_grid_roudtrip<GridD_QI>("GridD_QI", 8);
    test_one_grid_roudtrip<GridQ_LL>("GridQ_LL", 8);
    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 8);
    test_one_grid_roudtrip<GridDD_LL>("GridDD_LL", 8);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 8);

    test_one_grid_roudtrip<GridD_LL>("GridD_LL", 30);
    test_one_grid_roudtrip<GridD_QI>("GridD_QI", 30);
    test_one_grid_roudtrip<GridQ_LL>("GridQ_LL", 30);
    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 30);
    test_one_grid_roudtrip<GridDD_LL>("GridDD_LL", 30);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 30);

    test_one_grid_roudtrip<GridD_LL>("GridD_LL", 40);
    test_one_grid_roudtrip<GridD_QI>("GridD_QI", 40);
    test_one_grid_roudtrip<GridQ_LL>("GridQ_LL", 40);
    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 40);
    test_one_grid_roudtrip<GridDD_LL>("GridDD_LL", 40);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 40);

    test_one_grid_roudtrip<GridD_LL>("GridD_LL", 48);
    test_one_grid_roudtrip<GridD_QI>("GridD_QI", 48);
    test_one_grid_roudtrip<GridQ_LL>("GridQ_LL", 48);
    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 48);
    test_one_grid_roudtrip<GridDD_LL>("GridDD_LL", 48);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 48);

    test_one_grid_roudtrip<GridD_LL>("GridD_LL", 53);
    test_one_grid_roudtrip<GridD_QI>("GridD_QI", 53);
    test_one_grid_roudtrip<GridQ_LL>("GridQ_LL", 53);
    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 53);
    test_one_grid_roudtrip<GridDD_LL>("GridDD_LL", 53);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 53);

    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 63);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 63);

    test_one_grid_roudtrip<GridQ_QI>("GridQ_QI", 100);
    test_one_grid_roudtrip<GridDD_QI>("GridDD_QI", 100);
}

template <typename Scalar, typename Sint>
void testgrid(){
    Scalar a_1 = Scalar(-5.);
    Scalar b_1 = Scalar(10);
    Sint nBit_1 = Sint(15);

    QTGrid<Scalar, Sint> grid (a_1, b_1, nBit_1);
}

template <typename Scalar, typename Sint>
void test_save_load_roundtrip()
{
    Scalar a_1 = Scalar(-5.0);
    Scalar b_1 = Scalar(10.0);
    Sint nBit_1 = Sint(15);

    QTGrid<Scalar, Sint> grid(a_1, b_1, nBit_1);

    // ---------------- SAVE ----------------
    std::string filename = "grid_test";
    grid.save_json(filename);

    // ---------------- LOAD ----------------
    QTGrid<Scalar, Sint> grid2(filename + "_grid_E.json");

    // ---------------- CHECKS ----------------

    auto almost_equal = [](Scalar x, Scalar y) {
        using std::abs;
        using boost::multiprecision::abs;
        return abs(x - y) < Scalar(1e-12);
    };

    std::cout << "a: " << grid.get_a() << " vs " << grid2.get_a() << "\n";
    std::cout << "b: " << grid.get_b() << " vs " << grid2.get_b() << "\n";
    std::cout << "nBits: " << grid.get_nBits() << " vs " << grid2.get_nBits() << "\n";
    std::cout << "k_offset: " << grid.get_k_offset() << " vs " << grid2.get_k_offset() << "\n";
    std::cout << "x_ref: " << grid.get_x_ref() << " vs " << grid2.get_x_ref() << "\n";

    assert(almost_equal(grid.get_a(), grid2.get_a()));
    assert(almost_equal(grid.get_b(), grid2.get_b()));
    assert(grid.get_nBits() == grid2.get_nBits());
    assert(grid.get_k_offset() == grid2.get_k_offset());
    assert(almost_equal(grid.get_x_ref(), grid2.get_x_ref()));

    // derived consistency
    assert(grid2.get_N() == (Sint(1) << grid2.get_nBits()));

    Scalar dx1 = (grid.get_b() - grid.get_a()) / Scalar(grid.get_N());
    Scalar dx2 = (grid2.get_b() - grid2.get_a()) / Scalar(grid2.get_N());

    assert(almost_equal(dx1, dx2));

    std::cout << "QTGrid roundtrip test PASSED\n";
}

int main() {
    
    
    testgrid<double, long long>();
    testgrid<double, util::i128>();
    testgrid<float128, long long>();
    testgrid<float128, util::i128>();
    testgrid<dd_128, long long>();
    testgrid<dd_128, util::i128>();

    test_save_load_roundtrip<double, long long>();
    test_save_load_roundtrip<double, util::i128>();
    test_save_load_roundtrip<float128, long long>();
    test_save_load_roundtrip<float128, util::i128>();
    test_save_load_roundtrip<dd_128, long long>();
    test_save_load_roundtrip<dd_128, util::i128>();

    test_grid_roundtrip();
}
