#pragma once

#include "rs-core/arithmetic.hpp"
#include "rs-core/linear-algebra.hpp"
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace RS::Graphics {

    namespace Detail {

        template <std::floating_point T>
        constexpr int fast_floor(T x) noexcept {
            auto i = static_cast<int>(x);
            return i - static_cast<int>(x < static_cast<T>(i));
        }

        constexpr std::uint64_t lcg64_1(std::uint64_t x) noexcept {
            return x * 6'364'136'223'846'793'005ull + 1'442'695'040'888'963'407ull;
        }

        constexpr std::uint64_t lcg64_2(std::uint64_t x) noexcept {
            return x * 3'935'559'000'370'003'845ull + 8'831'144'850'135'198'739ull;
        }

    }

    // Noise class template

    template <std::floating_point T, std::size_t N> class Noise;

    // 2D noise

    template <std::floating_point T>
    class Noise<T, 2> {

    public:

        using seed_type = std::uint64_t;
        using value_type = T;
        using vector_type = Vector<T, 2>;

        static constexpr std::size_t dim = 2;

        Noise() = default;
        explicit Noise(std::uint64_t s) noexcept { seed(s); }

        T operator()(const vector_type& point) const noexcept;
        void seed(std::uint64_t s) noexcept;

    private:

        static constexpr int psize = 2048;
        static constexpr int pmask = psize - 1;
        static constexpr T scale1 = static_cast<T>(0.366'025'403'8);
        static constexpr T scale2 = static_cast<T>(0.211'324'865'4);

        struct grad { T dx, dy; };

        struct grad_table {
            grad grads[psize];
            grad_table() noexcept;
        };

        struct lattice_point {

            int xsv, ysv;
            T dx, dy;

            lattice_point() = default;

            lattice_point(int x, int y) noexcept {
                auto tx = static_cast<T>(x);
                auto ty = static_cast<T>(y);
                xsv = x;
                ysv = y;
                T ssv = - (tx + ty) * scale2;
                dx = - tx - ssv;
                dy = - ty - ssv;
            }

        };

        struct lattice_table {
            lattice_point points[32];
            lattice_table() noexcept;
        };

        int perm_[psize];
        grad grads_[psize];

    };

        template <std::floating_point T>
        void Noise<T, 2>::seed(std::uint64_t s) noexcept {

            static const grad_table gtable;

            int source[psize];

            for (auto i = 0; i < psize; i++) {
                source[i] = i;
            }

            for (auto i = psize - 1; i >= 0; i--) {
                s = Detail::lcg64_1(s);
                auto r = static_cast<int>((s + 31) % static_cast<std::uint64_t>(i + 1));
                perm_[i] = source[r];
                grads_[i] = gtable.grads[perm_[i]];
                source[r] = source[i];
            }

            (*this)(vector_type{});

        }

        template <std::floating_point T>
        T Noise<T, 2>::operator()(const vector_type& point) const noexcept {

            static const lattice_table lut;

            T s = scale1 * (point.x() + point.y());
            T xs = point.x() + s;
            T ys = point.y() + s;
            T value = T{0};
            int xsb = Detail::fast_floor(xs);
            int ysb = Detail::fast_floor(ys);
            T xsi = xs - T(xsb);
            T ysi = ys - T(ysb);
            auto a = static_cast<int>(xsi + ysi);

            int index = (a << 2)
                | (static_cast<int>(xsi - ysi / 2 + 1 - static_cast<T>(a) / 2) << 3)
                | (static_cast<int>(ysi - xsi / 2 + 1 - static_cast<T>(a) / 2) << 4);

            T ssi = - (xsi + ysi) * scale2;
            T xi = xsi + ssi;
            T yi = ysi + ssi;

            for (auto i = 0; i < 4; i++) {

                auto& c = lut.points[index + i];
                T dx = xi + c.dx;
                T dy = yi + c.dy;
                T attn = T{2} / T{3} - dx * dx - dy * dy;

                if (attn > 0) {
                    int pxm = (xsb + c.xsv) & pmask;
                    int pym = (ysb + c.ysv) & pmask;
                    grad g = grads_[perm_[pxm] ^ pym];
                    T extrapolation = g.dx * dx + g.dy * dy;
                    attn *= attn;
                    value += attn * attn * extrapolation;
                }

            }

            return value;

        }

        template <std::floating_point T>
        Noise<T, 2>::grad_table::grad_table() noexcept {

            static constexpr int n = 24;
            static constexpr T g1 = static_cast<T>(2.381'053'830);
            static constexpr T g2 = static_cast<T>(6.980'896'609);
            static constexpr T g3 = static_cast<T>(11.105'002'818);
            static constexpr T g4 = static_cast<T>(14.472'321'439);
            static constexpr T g5 = static_cast<T>(16.853'375'269);
            static constexpr T g6 = static_cast<T>(18.085'899'427);

            grad table[n] = {
                { + g1, + g6 }, { + g2, + g5 }, { + g3, + g4 }, { + g4, + g3 }, { + g5, + g2 }, { + g6, + g1 },
                { + g6, - g1 }, { + g5, - g2 }, { + g4, - g3 }, { + g3, - g4 }, { + g2, - g5 }, { + g1, - g6 },
                { - g1, - g6 }, { - g2, - g5 }, { - g3, - g4 }, { - g4, - g3 }, { - g5, - g2 }, { - g6, - g1 },
                { - g6, + g1 }, { - g5, + g2 }, { - g4, + g3 }, { - g3, + g4 }, { - g2, + g5 }, { - g1, + g6 },
            };

            for (auto i = 0; i < psize; i++) {
                grads[i] = table[i % n];
            }

        }

        template <std::floating_point T>
        Noise<T, 2>::lattice_table::lattice_table() noexcept {

            int i1, j1, i2, j2;

            for (auto i = 0; i < 8; i++) {

                if ((i & 1) == 0) {

                    if ((i & 2) == 0) {
                        i1 = -1;
                        j1 = 0;
                    } else {
                        i1 = 1;
                        j1 = 0;
                    }

                    if ((i & 4) == 0) {
                        i2 = 0;
                        j2 = -1;
                    } else {
                        i2 = 0;
                        j2 = 1;
                    }

                } else {

                    if ((i & 2) != 0) {
                        i1 = 2;
                        j1 = 1;
                    } else {
                        i1 = 0;
                        j1 = 1;
                    }

                    if ((i & 4) != 0) {
                        i2 = 1;
                        j2 = 2;
                    } else {
                        i2 = 1;
                        j2 = 0;
                    }

                }

                points[4 * i] = {0, 0};
                points[4 * i + 1] = {1, 1};
                points[4 * i + 2] = {i1, j1};
                points[4 * i + 3] = {i2, j2};

            }

        }

    // 3D noise

    template <std::floating_point T>
    class Noise<T, 3> {

    public:

        using seed_type = std::uint64_t;
        using value_type = T;
        using vector_type = Vector<T, 3>;

        static constexpr std::size_t dim = 3;

        Noise() = default;
        explicit Noise(std::uint64_t s) noexcept { seed(s); }

        T operator()(const vector_type& point) const noexcept;
        void seed(std::uint64_t s) noexcept;

    private:

        static constexpr int psize = 2048;
        static constexpr int pmask = psize - 1;

        struct grad { T dx, dy, dz; };

        struct lattice_point {

            T dxr, dyr, dzr;
            int xrv, yrv, zrv, fail, succ;

            lattice_point() = default;

            lattice_point(int x, int y, int z, int lattice) noexcept {
                auto offset1 = static_cast<T>(lattice) / T{2};
                auto offset2 = lattice * 1024;
                dxr = offset1 - T(x);
                dyr = offset1 - T(y);
                dzr = offset1 - T(z);
                xrv = offset2 + x;
                yrv = offset2 + y;
                zrv = offset2 + z;
            }

        };

        struct grad_table {
            grad grads[psize];
            grad_table() noexcept;
        };

        struct lattice_table {
            lattice_point points[112];
            lattice_table() noexcept;
        };

        int perm_[psize];
        grad grads_[psize];

    };

        template <std::floating_point T>
        void Noise<T, 3>::seed(std::uint64_t s) noexcept {

            static const grad_table gtable;

            int source[psize];

            for (auto i = 0; i < psize; i++) {
                source[i] = i;
            }

            for (auto i = psize - 1; i >= 0; i--) {
                s = Detail::lcg64_1(s);
                auto r = static_cast<int>((s + 31) % static_cast<std::uint64_t>(i + 1));
                perm_[i] = source[r];
                grads_[i] = gtable.grads[perm_[i]];
                source[r] = source[i];
            }

            (*this)(vector_type{});

        }

        template <std::floating_point T>
        T Noise<T, 3>::operator()(const vector_type& point) const noexcept {

            static const lattice_table lut;

            T r = T{2} / T{3} * (point.x() + point.y() + point.z());
            T xr = r - point.x();
            T yr = r - point.y();
            T zr = r - point.z();
            int xrb = Detail::fast_floor(xr);
            int yrb = Detail::fast_floor(yr);
            int zrb = Detail::fast_floor(zr);
            T xri = xr - T(xrb);
            T yri = yr - T(yrb);
            T zri = zr - T(zrb);
            int xht = int(xri + T(0.5));
            int yht = int(yri + T(0.5));
            int zht = int(zri + T(0.5));
            int index = xht | (yht << 1) | (zht << 2);
            T value = T{0};
            int ci = 14 * index;

            while (ci != -1) {

                auto& c = lut.points[ci];
                T dxr = xri + c.dxr;
                T dyr = yri + c.dyr;
                T dzr = zri + c.dzr;
                T attn = static_cast<T>(0.75) - dxr * dxr - dyr * dyr - dzr * dzr;

                if (attn < T{0}) {

                    ci = c.fail;

                } else {

                    int pxm = (xrb + c.xrv) & pmask;
                    int pym = (yrb + c.yrv) & pmask;
                    int pzm = (zrb + c.zrv) & pmask;
                    grad g = grads_[perm_[perm_[pxm] ^ pym] ^ pzm];
                    T extrapolation = g.dx * dxr + g.dy * dyr + g.dz * dzr;
                    attn *= attn;
                    value += attn * attn * extrapolation;
                    ci = c.succ;

                }

            }

            return value;

        }

        template <std::floating_point T>
        Noise<T, 3>::grad_table::grad_table() noexcept {

            static constexpr int n = 48;
            static constexpr T g1 = static_cast<T>(3.594'631'769);
            static constexpr T g2 = static_cast<T>(4.213'452'452);
            static constexpr T g3 = static_cast<T>(7.997'138'591);
            static constexpr T g4 = static_cast<T>(11.093'991'497);

            static constexpr grad table[n] = {
                { - g3, - g3, - g1 }, { - g3, - g3, + g1 }, { - g4, - g2, T{0} }, { - g2, - g4, T{0} },
                { - g3, - g1, - g3 }, { - g3, + g1, - g3 }, { - g2, T{0}, - g4 }, { - g4, T{0}, - g2 },
                { - g3, - g1, + g3 }, { - g3, + g1, + g3 }, { - g4, T{0}, + g2 }, { - g2, T{0}, + g4 },
                { - g3, + g3, - g1 }, { - g3, + g3, + g1 }, { - g2, + g4, T{0} }, { - g4, + g2, T{0} },
                { - g1, - g3, - g3 }, { + g1, - g3, - g3 }, { T{0}, - g4, - g2 }, { T{0}, - g2, - g4 },
                { - g1, - g3, + g3 }, { + g1, - g3, + g3 }, { T{0}, - g2, + g4 }, { T{0}, - g4, + g2 },
                { - g1, + g3, - g3 }, { + g1, + g3, - g3 }, { T{0}, + g2, - g4 }, { T{0}, + g4, - g2 },
                { - g1, + g3, + g3 }, { + g1, + g3, + g3 }, { T{0}, + g4, + g2 }, { T{0}, + g2, + g4 },
                { + g3, - g3, - g1 }, { + g3, - g3, + g1 }, { + g2, - g4, T{0} }, { + g4, - g2, T{0} },
                { + g3, - g1, - g3 }, { + g3, + g1, - g3 }, { + g4, T{0}, - g2 }, { + g2, T{0}, - g4 },
                { + g3, - g1, + g3 }, { + g3, + g1, + g3 }, { + g2, T{0}, + g4 }, { + g4, T{0}, + g2 },
                { + g3, + g3, - g1 }, { + g3, + g3, + g1 }, { + g4, + g2, T{0} }, { + g2, + g4, T{0} },
            };

            for (auto i = 0; i < psize; i++) {
                grads[i] = table[i % n];
            }

        }

        template <std::floating_point T>
        Noise<T, 3>::lattice_table::lattice_table() noexcept {

            for (auto i = 0; i < 8; i++) {

                int i1 = (i >> 0) & 1;
                int i2 = i1 ^ 1;
                int j1 = (i >> 1) & 1;
                int j2 = j1 ^ 1;
                int k1 = (i >> 2) & 1;
                int k2 = k1 ^ 1;
                int ci = 14 * i;
                auto c = points + ci;

                c[0] = {i1, j1, k1, 0};
                c[1] = {i1 + i2, j1 + j2, k1 + k2, 1};
                c[2] = {i1 ^ 1, j1, k1, 0};
                c[3] = {i1, j1 ^ 1, k1 ^ 1, 0};
                c[4] = {i1 + (i2 ^ 1), j1 + j2, k1 + k2, 1};
                c[5] = {i1 + i2, j1 + (j2 ^ 1), k1 + (k2 ^ 1), 1};
                c[6] = {i1, j1 ^ 1, k1, 0};
                c[7] = {i1 ^ 1, j1, k1 ^ 1, 0};
                c[8] = {i1 + i2, j1 + (j2 ^ 1), k1 + k2, 1};
                c[9] = {i1 + (i2 ^ 1), j1 + j2, k1 + (k2 ^ 1), 1};
                c[10] = {i1, j1, k1 ^ 1, 0};
                c[11] = {i1 ^ 1, j1 ^ 1, k1, 0};
                c[12] = {i1 + i2, j1 + j2, k1 + (k2 ^ 1), 1};
                c[13] = {i1 + (i2 ^ 1), j1 + (j2 ^ 1), k1 + k2, 1};

                c[0].fail = c[0].succ = ci + 1;
                c[1].fail = c[1].succ = ci + 2;
                c[2].fail = ci + 3;
                c[2].succ = ci + 5;
                c[3].fail = ci + 4;
                c[3].succ = ci + 4;
                c[4].fail = ci + 5;
                c[4].succ = ci + 6;
                c[5].fail = c[5].succ = ci + 6;
                c[6].fail = ci + 7;
                c[6].succ = ci + 9;
                c[7].fail = ci + 8;
                c[7].succ = ci + 8;
                c[8].fail = ci + 9;
                c[8].succ = ci + 10;
                c[9].fail = c[9].succ = ci + 10;
                c[10].fail = ci + 11;
                c[10].succ = ci + 13;
                c[11].fail = ci + 12;
                c[11].succ = ci + 12;
                c[12].fail = ci + 13;
                c[12].succ = c[13].fail = c[13].succ = -1;

            }

        }

    // Generalised noise source

    template <std::floating_point T, std::size_t DimIn, std::size_t DimOut>
    class NoiseSource {

    public:

        using seed_type = std::uint64_t;
        using value_type = T;
        using domain_type = std::conditional_t<DimIn == 1, T, Vector<T, DimIn>>;
        using result_type = std::conditional_t<DimOut == 1, T, Vector<T, DimOut>>;

        static constexpr std::size_t dim_in = DimIn;
        static constexpr std::size_t dim_out = DimOut;

        NoiseSource() = default;
        explicit NoiseSource(T cell, T scale, std::size_t octaves, std::uint64_t seed) noexcept;

        result_type operator()(domain_type point) const noexcept;

        T cell() const noexcept { return cell_; }
        void cell(T size) noexcept { cell_ = std::abs(size); }
        std::size_t octaves() const noexcept { return octaves_; }
        void octaves(std::size_t n) noexcept { octaves_ = n; }
        T scale() const noexcept { return scale_; }
        void scale(T factor) noexcept { scale_ = std::abs(factor); }
        void seed(std::uint64_t s) noexcept;

    private:

        using noise_type = Noise<T, std::max(DimIn, 2uz)>;
        using input_vector = typename noise_type::vector_type;

        noise_type generators_[DimOut];
        T cell_ = T{1};
        T scale_ = T{1};
        std::size_t octaves_ = 1;

    };

        template <std::floating_point T, std::size_t DimIn, std::size_t DimOut>
        NoiseSource<T, DimIn, DimOut>::NoiseSource(T cell, T scale, std::size_t octaves, std::uint64_t s) noexcept:
        generators_(), cell_(std::abs(cell)), scale_(std::abs(scale)), octaves_(octaves) {
            seed(s);
        }

        template <std::floating_point T, std::size_t DimIn, std::size_t DimOut>
        typename NoiseSource<T, DimIn, DimOut>::result_type NoiseSource<T, DimIn, DimOut>::operator()(domain_type point) const noexcept {

            point /= cell_;
            input_vector in;

            if constexpr (DimIn == 1) {
                in = {point, T{0}};
            } else{
                in = point;
            }

            result_type out(T{0});
            T s = scale_;

            for (auto i = 0uz; i < octaves_; ++i, in *= 2, s /= 2) {
                if constexpr (DimOut == 1) {
                    out += s * generators_[0](in);
                } else {
                    for (auto j = 0uz; j < DimOut; ++j) {
                        out[j] += s * generators_[j](in);
                    }
                }
            }

            return out;

        }

        template <std::floating_point T, std::size_t DimIn, std::size_t DimOut>
        void NoiseSource<T, DimIn, DimOut>::seed(std::uint64_t s) noexcept {
            for (auto& gen: generators_) {
                gen.seed(s);
                s = Detail::lcg64_2(s);
            }
        }

}
