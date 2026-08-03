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

using json = nlohmann::json;

namespace qtgrid_json_detail {

inline std::string double_to_exact_string(double value)
{
    std::ostringstream oss;
    oss << std::scientific
        << std::setprecision(std::numeric_limits<double>::max_digits10)
        << value;
    return oss.str();
}

inline json encode_dd128_exact(const dd_128& value)
{
    return {
        {"hi", double_to_exact_string(value.x[0])},
        {"lo", double_to_exact_string(value.x[1])}
    };
}

inline double parse_exact_double(const json& value)
{
    double result;
    std::istringstream input(value.get<std::string>());
    input >> result;
    if (!input) {
        throw std::runtime_error("Invalid exact double in grid JSON");
    }
    return result;
}

inline dd_128 decode_dd128_exact(const json& value)
{
    const double hi = parse_exact_double(value.at("hi"));
    const double lo = parse_exact_double(value.at("lo"));
    return dd_128(dd_real(hi, lo));
}

} // namespace qtgrid_json_detail

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

        if constexpr (std::is_same_v<Scalar, dd_128>) {
            const json exact = {
                {"a", qtgrid_json_detail::encode_dd128_exact(a)},
                {"b", qtgrid_json_detail::encode_dd128_exact(b)}
            };
            // Keep both legacy representations and add exact dd_128 data.
            j["dd128_exact"] = exact;
            j_2["dd128_exact"] = exact;
        }

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

        if constexpr (std::is_same_v<Scalar, dd_128>) {
            if (j.contains("dd128_exact")) {
                const auto& exact = j.at("dd128_exact");
                a = qtgrid_json_detail::decode_dd128_exact(exact.at("a"));
                b = qtgrid_json_detail::decode_dd128_exact(exact.at("b"));
                return {a, b, nBits};
            }
        }

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


// MultQTGrid: multi-dimensional extension of QTGrid.
// Represents a dim-dimensional uniform grid on [a[0],b[0]) x ... x [a[dim-1],b[dim-1])
// with 2^nBits[i] points along each dimension i.
// Bits from each dimension are concatenated (not interleaved), so tensorLen = sum(nBits).
template <typename Scalar, typename Sint>
class MultQTGrid {
public:
    using scalar_type = Scalar;
    using index_type = Sint;

private:
    int dim_;
    std::vector<Scalar> a;
    std::vector<Scalar> b;
    std::vector<int> nBits;
    std::vector<Sint> N;
    std::vector<Scalar> dx;
    int tensorLen_;
    Scalar deltaVolume_;
    std::vector<int> bitOffsets;  // bitOffsets[i] = sum_{j<i} nBits[j]

public:
    // --- read-only accessors ---
    int                     get_dim()       const { return dim_; }
    const std::vector<Scalar>& get_a()      const { return a; }
    const std::vector<Scalar>& get_b()      const { return b; }
    const std::vector<int>&    get_nBits()  const { return nBits; }
    const std::vector<Sint>&   get_N()      const { return N; }
    const std::vector<Scalar>& get_dx()     const { return dx; }
    int                     get_tensorLen() const { return tensorLen_; }
    Scalar                  delta_volume()  const { return deltaVolume_; }

    MultQTGrid(std::vector<Scalar> a_, std::vector<Scalar> b_, std::vector<int> nBits_)
        : dim_(static_cast<int>(a_.size()))
        , a(std::move(a_))
        , b(std::move(b_))
        , nBits(std::move(nBits_))
    {
        if (dim_ == 0)
            throw std::invalid_argument("MultQTGrid: dimension must be > 0");
        if (b.size() != static_cast<size_t>(dim_) || nBits.size() != static_cast<size_t>(dim_))
            throw std::invalid_argument("MultQTGrid: a, b, nBits must have the same size");

        N.resize(dim_);
        dx.resize(dim_);
        bitOffsets.resize(dim_ + 1);
        tensorLen_ = 0;
        deltaVolume_ = Scalar(1);

        for (int i = 0; i < dim_; ++i) {
            if (nBits[i] < 0 || nBits[i] > 126)
                throw std::invalid_argument("nBits[" + std::to_string(i) + "] must be in [0, 126]");
            N[i] = Sint(1) << nBits[i];
            dx[i] = (b[i] - a[i]) / Scalar(N[i]);
            deltaVolume_ *= dx[i];
            bitOffsets[i] = tensorLen_;
            tensorLen_ += nBits[i];
        }
        bitOffsets[dim_] = tensorLen_;
    }

    MultQTGrid(const std::string& filename)
    {
        auto [a_, b_, nBits_] = parse_json(filename);
        *this = MultQTGrid(std::move(a_), std::move(b_), std::move(nBits_));
    }

    std::vector<int> coord_to_id(std::vector<Scalar> const& xs) const {
        if (static_cast<int>(xs.size()) != dim_)
            throw std::invalid_argument("coord_to_id: expected " + std::to_string(dim_) + " coordinates, got " + std::to_string(xs.size()));
        std::vector<int> bits(tensorLen_, 0);
        for (int i = 0; i < dim_; ++i) {
            Scalar t = (xs[i] - a[i]) * Scalar(N[i]) / (b[i] - a[i]);
            Sint k = round_to_sint(t);
            if (k == N[i]) k = N[i] - 1;
            if (k < 0 || k > N[i] - 1)
                throw std::out_of_range("coord_to_id: k[" + std::to_string(i) + "] is outside [0, N-1]");
            int off = bitOffsets[i];
            for (int d = 0; d < nBits[i]; ++d)
                bits[off + d] = (k >> d) & 1;
        }
        return bits;
    }

    std::vector<Scalar> id_to_coord(std::vector<int> const& bits) const {
        if (static_cast<int>(bits.size()) != tensorLen_)
            throw std::invalid_argument("id_to_coord: expected " + std::to_string(tensorLen_) + " bits, got " + std::to_string(bits.size()));
        std::vector<Scalar> xs(dim_);
        for (int i = 0; i < dim_; ++i) {
            Sint k = 0;
            int off = bitOffsets[i];
            for (int d = 0; d < nBits[i]; ++d)
                k |= (Sint(bits[off + d]) << d);
            xs[i] = a[i] + Scalar(k) * dx[i];
        }
        return xs;
    }

    void save_json(const std::string& base) const
    {
        json j;
        json j_2;

        auto to_double_lossy = [](const Scalar& v) -> double {
            if constexpr (std::is_same_v<Scalar, dd_128>) {
                return v._hi();
            } else {
                return static_cast<double>(v);
            }
        };

        // lossy JSON
        std::vector<double> a_lossy(dim_), b_lossy(dim_);
        for (int i = 0; i < dim_; ++i) {
            a_lossy[i] = to_double_lossy(a[i]);
            b_lossy[i] = to_double_lossy(b[i]);
        }
        j["a"] = a_lossy;
        j["b"] = b_lossy;
        j["nBits"] = nBits;

        // exact JSON
        std::vector<std::string> a_exact(dim_), b_exact(dim_);
        for (int i = 0; i < dim_; ++i) {
            std::ostringstream oss_a, oss_b;
            oss_a << std::setprecision(std::numeric_limits<Scalar>::digits10 + 5) << a[i];
            oss_b << std::setprecision(std::numeric_limits<Scalar>::digits10 + 5) << b[i];
            a_exact[i] = oss_a.str();
            b_exact[i] = oss_b.str();
        }
        j_2["a"] = a_exact;
        j_2["b"] = b_exact;
        j_2["nBits"] = nBits;

        if constexpr (std::is_same_v<Scalar, dd_128>) {
            json exact_a = json::array();
            json exact_b = json::array();
            for (int i = 0; i < dim_; ++i) {
                exact_a.push_back(qtgrid_json_detail::encode_dd128_exact(a[i]));
                exact_b.push_back(qtgrid_json_detail::encode_dd128_exact(b[i]));
            }

            const json exact = {{"a", exact_a}, {"b", exact_b}};
            // Keep both legacy representations and add exact dd_128 data.
            j["dd128_exact"] = exact;
            j_2["dd128_exact"] = exact;
        }

        std::ofstream file(base + "_multgrid.json");
        if (!file.is_open())
            throw std::runtime_error("Cannot open file");
        std::ofstream file_2(base + "_multgrid_E.json");
        if (!file_2.is_open())
            throw std::runtime_error("Cannot open file_2");
        file << j.dump(4);
        file_2 << j_2.dump(4);
    }

private:

    static Sint round_to_sint(Scalar t) {
        using std::floor;
        using std::ceil;
        Scalar r = (t < Scalar(0)) ? ceil(t - Scalar(0.5))
                                   : floor(t + Scalar(0.5));
        if constexpr (std::is_same_v<Scalar, dd_128>) {
            return static_cast<Sint>(r._hi()) + static_cast<Sint>(r._lo());
        } else if constexpr (std::is_arithmetic_v<Sint> ||
                             std::is_same_v<Sint, __int128>) {
            return static_cast<Sint>(r);
        } else {
            return r.template convert_to<Sint>();
        }
    }

    static std::tuple<std::vector<Scalar>, std::vector<Scalar>, std::vector<int>>
    parse_json(const std::string& filename)
    {
        json j;
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file");
        file >> j;

        std::vector<int> nBits = j.at("nBits").get<std::vector<int>>();
        int dim = static_cast<int>(nBits.size());
        std::vector<Scalar> a(dim), b(dim);

        if constexpr (std::is_same_v<Scalar, dd_128>) {
            if (j.contains("dd128_exact")) {
                const auto& exact = j.at("dd128_exact");
                const auto& exact_a = exact.at("a");
                const auto& exact_b = exact.at("b");
                if (exact_a.size() != static_cast<std::size_t>(dim) ||
                    exact_b.size() != static_cast<std::size_t>(dim)) {
                    throw std::runtime_error("Invalid dd_128 exact multigrid dimensions");
                }

                for (int i = 0; i < dim; ++i) {
                    a[i] = qtgrid_json_detail::decode_dd128_exact(exact_a[i]);
                    b[i] = qtgrid_json_detail::decode_dd128_exact(exact_b[i]);
                }
                return {std::move(a), std::move(b), std::move(nBits)};
            }
        }

        bool a_is_string = j["a"].is_array() && j["a"].size() > 0 && j["a"][0].is_string();
        bool b_is_string = j["b"].is_array() && j["b"].size() > 0 && j["b"][0].is_string();

        for (int i = 0; i < dim; ++i) {
            if (a_is_string) {
                std::istringstream(j["a"][i].get<std::string>()) >> a[i];
            } else {
                a[i] = Scalar(j["a"][i].get<double>());
            }
            if (b_is_string) {
                std::istringstream(j["b"][i].get<std::string>()) >> b[i];
            } else {
                b[i] = Scalar(j["b"][i].get<double>());
            }
        }
        return {std::move(a), std::move(b), std::move(nBits)};
    }
};
