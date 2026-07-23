#pragma once
#include <cmath>
#include <vector>
#include <tuple>
#include <limits>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <nlohmann/json.hpp>

#include "type_double_double.h"
#include "type_float128_boost.h"

struct dd_128;  // fwd decl for lossy JSON double conversion in save_json

using json = nlohmann::json;

// QTGrid supports up to nBits = 126 (limited by Sint width: Sint(1) << nBits
// must fit in a signed Sint). Scalar must carry enough mantissa bits for the
// coordinate transform; see precision note in coord_to_id.
template <typename Scalar, typename Sint>
class QTGrid {
public:
    using scalar_type = Scalar;
    using index_type = Sint;

private:
    Scalar a;
    Scalar b;
    int nBits;
    Sint N;
    Scalar dx;
    Sint k_offset;
    Scalar x_ref;

public:
    // --- read-only accessors ---
    const Scalar& get_a()        const { return a; }
    const Scalar& get_b()        const { return b; }
    int           get_nBits()    const { return nBits; }
    Sint          get_N()        const { return N; }
    const Scalar& get_dx()       const { return dx; }
    Sint          get_k_offset() const { return k_offset; }
    Scalar        get_x_ref()    const { return x_ref; }

    // Primary constructor: k_offset defaults to 0, x_ref defaults to a.
    // The two-overload pattern avoids ambiguity while letting callers
    // supply an explicit x_ref (matching XFAC_1d_qgrid's signature).
    QTGrid(Scalar a_, Scalar b_, int nBits_,
           Sint k_offset_ = 0)
        : QTGrid(a_, b_, nBits_, k_offset_, a_)
    {}

    QTGrid(Scalar a_, Scalar b_, int nBits_,
           Sint k_offset_,
           Scalar x_ref_)
        : a(a_), b(b_), nBits(nBits_), k_offset(k_offset_), x_ref(x_ref_)
    {
        if (nBits < 0 || nBits > 126) {
            throw std::invalid_argument("nBits must be in [0, 126]");
        }
        N = Sint(1) << nBits;
        dx = (b_ - a_) / Scalar(N);
    }

    QTGrid(const std::string& filename)
    {
        auto [a_, b_, nBits_] = parse_json(filename);
        *this = QTGrid(a_, b_, nBits_);
    }

    std::vector<int> coord_to_id(Scalar x) const {
        // Note: for large nBits this transform consumes nBits of Scalar's
        // mantissa; with float128 (113-bit mantissa) and nBits = 100 only
        // ~13 fractional bits remain, so points very close to cell
        // boundaries may land in the neighboring cell.
        Scalar t = (x - a) * Scalar(N) / (b - a);
        Sint k = round_to_sint(t);
        if (k == N) k = N - 1;
        if (k < 0 || k > N - 1)
            throw std::out_of_range("k is outside [0, N-1]");
        return _bits(k);
    }

    Scalar id_to_coord(const std::vector<int>& bits) const {
        Sint k = 0;
        for (int i = 0; i < nBits; ++i)
            k |= (Sint(bits[i]) << i);
        return _coords(k);
    }

    void save_json(const std::string& base) const
    {
        json j;
        json j_2;
        std::string suffix;
        std::string suffix_2;

        auto to_double_lossy = [](const Scalar& v) -> double {
            if constexpr (std::is_same_v<Scalar, dd_128>) {
                return v._hi();
            } else {
                return static_cast<double>(v);
            }
        };

        j["a"] = to_double_lossy(a);
        j["b"] = to_double_lossy(b);
        suffix = "_grid.json";
        j["nBits"] = nBits;

        std::ostringstream oss_a;
        oss_a << std::setprecision(std::numeric_limits<Scalar>::digits10 + 5) << a;
        j_2["a"] = oss_a.str();
        std::ostringstream oss_b;
        oss_b << std::setprecision(std::numeric_limits<Scalar>::digits10 + 5) << b;
        j_2["b"] = oss_b.str();
        suffix_2 = "_grid_E.json";
        j_2["nBits"] = nBits;

        std::ofstream file(base + suffix);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file");
        std::ofstream file_2(base + suffix_2);
        if (!file_2.is_open())
            throw std::runtime_error("Cannot open file_2");
        file << j.dump(4);
        file_2 << j_2.dump(4);
    }

    Scalar delta_volume() const {
        return dx;
    }

    void update_padding_1h_bit() {
        // Increase range by 1 high bit: a and dx unchanged,
        // b doubles the interval, nBits and N increase.
        if (nBits + 1 > 126)
            throw std::overflow_error("update_padding_1h_bit: nBits would exceed 126");
        b = b + (b - a);
        nBits = nBits + 1;
        N = Sint(1) << nBits;
    }

    void update_padding_1l_bit() {
        // Increase resolution by 1 low bit: a and b unchanged,
        // dx halves, nBits and N increase.
        if (nBits + 1 > 126)
            throw std::overflow_error("update_padding_1l_bit: nBits would exceed 126");
        nBits = nBits + 1;
        N = Sint(1) << nBits;
        dx = (b - a) / Scalar(N);
    }

    QTGrid build_dual_grid(bool centered = true) const {
        Scalar L = b - a;
        // df = 2*pi / L
        Scalar df = Scalar(2) * pi<Scalar>() / L;
        Scalar Lf = Scalar(N) * df;
        if (centered) {
            return QTGrid(-Lf / Scalar(2), Lf / Scalar(2), nBits,
                          N / Sint(2), Scalar(0));
        } else {
            return QTGrid(Scalar(0), Lf, nBits,
                          Sint(0), Scalar(0));
        }
    }

private:

    static Sint round_to_sint(Scalar t) {
        using std::floor;
        using std::ceil;
        Scalar r = (t < Scalar(0)) ? ceil(t - Scalar(0.5))
                                   : floor(t + Scalar(0.5));
        if constexpr (std::is_same_v<Scalar, dd_128>) {
            // After a correct dd floor/ceil, hi and lo are both
            // integer-valued doubles; sum them exactly in Sint.
            // (lo may be negative — the addition handles that.)
            return static_cast<Sint>(r._hi()) + static_cast<Sint>(r._lo());
        } else if constexpr (std::is_arithmetic_v<Sint> ||
                             std::is_same_v<Sint, __int128>) {
            return static_cast<Sint>(r);
        } else {
            return r.template convert_to<Sint>();
        }
    }

    static std::tuple<Scalar, Scalar, int> parse_json(const std::string& filename)
    {
        json j;
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file");
        file >> j;
        int nBits = j.at("nBits").get<int>();
        Scalar a, b;
        if (j["a"].is_string())
        {
            std::istringstream(j.at("a").get<std::string>()) >> a;
            std::istringstream(j.at("b").get<std::string>()) >> b;
        }
        else
        {
            a = Scalar(j.at("a").get<double>());
            b = Scalar(j.at("b").get<double>());
        }
        return {a, b, nBits};
    }

    std::vector<int> _bits(Sint k) const {
        std::vector<int> bits(nBits, 0);
        for (int i = 0; i < nBits; ++i)
            bits[i] = (k >> i) & 1;
        return bits;
    }

    Scalar _coords(Sint k) const {
        return x_ref + Scalar(k - k_offset) * dx;
    }
};
