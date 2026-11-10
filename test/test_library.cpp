#include <QTgrid-quad/grid_alias.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<typename Scalar, typename Index>
void check_specialization(const std::string& name) {
    const QTGrid<Scalar, Index> grid(Scalar(0), Scalar(1), 8);
    const Scalar coordinate(0.25);
    const auto bits = grid.coord_to_id(coordinate);
    require(bits.size() == 8, "grid bit count");
    require(grid.id_to_coord(bits) == coordinate, "grid roundtrip");
    require(grid.get_N() == Index(256), "grid size");
    require(grid.delta_volume() == Scalar(1) / Scalar(256), "grid spacing");

    const auto dual = grid.build_dual_grid();
    require(dual.get_nBits() == 8 && dual.get_k_offset() == Index(128), "dual grid");
    auto padded = grid;
    padded.update_padding_1h_bit();
    require(padded.get_N() == Index(512) && padded.get_b() == Scalar(2), "high padding");
    padded.update_padding_1l_bit();
    require(padded.get_N() == Index(1024), "low padding");
    require(padded.get_dx() == grid.get_dx() / Scalar(2), "padded spacing");

    const std::string base = "compiled_" + name;
    grid.save_json(base);
    const QTGrid<Scalar, Index> loaded(base + "_grid_E.json");
    require(loaded.get_a() == grid.get_a() && loaded.get_b() == grid.get_b(), "grid JSON bounds");
    require(loaded.get_N() == grid.get_N(), "grid JSON size");

    const MultQTGrid<Scalar, Index> multi(
        {Scalar(0), Scalar(-1)}, {Scalar(1), Scalar(1)}, {8, 4});
    const std::vector<Scalar> coordinates{coordinate, Scalar(0.5)};
    const auto multi_bits = multi.coord_to_id(coordinates);
    require(multi.get_dim() == 2 && multi.get_tensorLen() == 12, "multigrid dimensions");
    require(multi.id_to_coord(multi_bits) == coordinates, "multigrid roundtrip");
    multi.save_json(base);
    const MultQTGrid<Scalar, Index> multi_loaded(base + "_multgrid_E.json");
    require(multi_loaded.get_a() == multi.get_a() && multi_loaded.get_b() == multi.get_b(),
            "multigrid JSON bounds");
    require(multi_loaded.delta_volume() == multi.delta_volume(), "multigrid volume");
}

template<typename Scalar>
void check_indices(const std::string& name) {
    check_specialization<Scalar, int>(name + "_int");
    check_specialization<Scalar, long>(name + "_long");
    check_specialization<Scalar, long long>(name + "_long_long");
    check_specialization<Scalar, util::i128>(name + "_i128");
}

} // namespace

int main() {
    try {
        check_indices<double>("double");
        check_indices<dd_128>("dd128");
        check_indices<float128>("float128");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
