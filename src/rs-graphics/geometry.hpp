#pragma once

#include "rs-core/arithmetic.hpp"
#include "rs-core/linear-algebra.hpp"
#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <numbers>

namespace RS::Graphics {

    template <Scalar T, std::size_t N>
    class Box {

    public:

        using value_type = T;
        using vector_type = Vector<T, N>;

        static constexpr std::size_t dim = N;

        constexpr Box() noexcept: corner_(T{0}), extent_(T{0}) {}
        constexpr explicit Box(const vector_type& p, const vector_type& v) noexcept;

        constexpr vector_type base() const noexcept { return corner_; }
        constexpr vector_type apex() const noexcept { return corner_ + extent_; }
        constexpr vector_type centre() const noexcept { return corner_ + extent_ / T{2}; }
        constexpr bool contains(const vector_type& p) const noexcept;
        constexpr bool contains(const Box& b) const noexcept;
        constexpr bool empty() const noexcept;
        constexpr T volume() const noexcept;
        constexpr vector_type shape() const noexcept { return extent_; }

        constexpr Box& operator+=(const vector_type& v) noexcept { corner_ += v; return *this; }
        constexpr Box& operator-=(const vector_type& v) noexcept { corner_ -= v; return *this; }
        constexpr friend Box operator+(const Box& a, const vector_type& b) noexcept { Box c = a; c += b; return c; }
        constexpr friend Box operator+(const vector_type& a, const Box& b) noexcept { Box c = b; c += a; return c; }
        constexpr friend Box operator-(const Box& a, const vector_type& b) noexcept { Box c = a; c -= b; return c; }
        constexpr friend bool operator==(const Box& a, const Box& b) noexcept = default;

        constexpr static Box unit() noexcept { return Box(vector_type::null(), vector_type{T{1}}); }

    private:

        vector_type corner_;
        vector_type extent_;

    };

        template <Scalar T, std::size_t N>
        constexpr Box<T, N>::Box(const vector_type& p, const vector_type& v) noexcept:
        corner_(p), extent_(v) {
            for (auto i = 0uz; i < N; ++i) {
                if (extent_[i] < T{0}) {
                    corner_[i] += extent_[i];
                    extent_[i] = - extent_[i];
                }
            }
        }

        template <Scalar T, std::size_t N>
        constexpr bool Box<T, N>::contains(const vector_type& p) const noexcept {
            for (auto i = 0uz; i < N; ++i) {
                if (p[i] < corner_[i] || p[i] >= corner_[i] + extent_[i]) {
                    return false;
                }
            }
            return true;
        }

        template <Scalar T, std::size_t N>
        constexpr bool Box<T, N>::contains(const Box& b) const noexcept {
            for (auto i = 0uz; i < N; ++i) {
                if (b.corner_[i] < corner_[i] || b.corner_[i] + b.extent_[i] > corner_[i] + extent_[i]) {
                    return false;
                }
            }
            return true;
        }

        template <Scalar T, std::size_t N>
        constexpr bool Box<T, N>::empty() const noexcept {
            for (auto x: extent_) {
                if (x == T{0}) {
                    return true;
                }
            }
            return false;
        }

        template <Scalar T, std::size_t N>
        constexpr T Box<T, N>::volume() const noexcept {
            T m{1};
            for (auto x: extent_) {
                m *= x;
            }
            return m;
        }

    using Box_i2 = Box<int, 2>;
    using Box_i3 = Box<int, 3>;
    using Box_i4 = Box<int, 4>;
    using Box_s2 = Box<std::size_t, 2>;
    using Box_s3 = Box<std::size_t, 3>;
    using Box_s4 = Box<std::size_t, 4>;
    using Box_f2 = Box<float, 2>;
    using Box_f3 = Box<float, 3>;
    using Box_f4 = Box<float, 4>;
    using Box_d2 = Box<double, 2>;
    using Box_d3 = Box<double, 3>;
    using Box_d4 = Box<double, 4>;
    using Box_ld2 = Box<long double, 2>;
    using Box_ld3 = Box<long double, 3>;
    using Box_ld4 = Box<long double, 4>;

    template <std::floating_point T, std::size_t N>
    class Sphere {

    public:

        using value_type = T;
        using vector_type = Vector<T, N>;

        static constexpr std::size_t dim = N;

        constexpr Sphere() noexcept = default;
        constexpr explicit Sphere(const vector_type& c, T r) noexcept: centre_{c}, radius_{std::abs(r)} {}

        constexpr vector_type centre() const noexcept { return centre_; }
        constexpr T radius() const noexcept { return radius_; }
        constexpr bool contains(const vector_type& p) const noexcept { return (p - centre_).r2() < radius_ * radius_; }
        constexpr bool contains(const Sphere& s) const noexcept;
        constexpr bool disjoint(const Sphere& s) const noexcept;
        constexpr bool empty() const noexcept { return radius_ == T{0}; }
        T volume() const noexcept;
        T surface() const noexcept;

        constexpr Sphere& operator+=(const vector_type& v) noexcept { centre_ += v; return *this; }
        constexpr Sphere& operator-=(const vector_type& v) noexcept { centre_ -= v; return *this; }
        constexpr Sphere& operator*=(T t) noexcept { radius_ *= t; return *this; }
        constexpr Sphere& operator/=(T t) noexcept { radius_ /= t; return *this; }
        constexpr friend Sphere operator+(const Sphere& a, const vector_type& b) noexcept { Sphere c = a; c += b; return c; }
        constexpr friend Sphere operator+(const vector_type& a, const Sphere& b) noexcept { Sphere c = b; c += a; return c; }
        constexpr friend Sphere operator-(const Sphere& a, const vector_type& b) noexcept { Sphere c = a; c -= b; return c; }
        constexpr friend Sphere operator*(const Sphere& a, T b) noexcept { Sphere c = a; c *= b; return c; }
        constexpr friend Sphere operator*(T a, const Sphere& b) noexcept { Sphere c = b; c *= a; return c; }
        constexpr friend Sphere operator/(const Sphere& a, T b) noexcept { Sphere c = a; c /= b; return c; }
        constexpr friend bool operator==(const Sphere& a, const Sphere& b) noexcept = default;

        constexpr static Sphere unit() noexcept { return Sphere(vector_type::null(), T{1}); }

    private:

        vector_type centre_;
        T radius_ = T{0};

    };

        template <std::floating_point T, std::size_t N>
        constexpr bool Sphere<T, N>::contains(const Sphere& s) const noexcept {
            if (s.radius_ >= radius_) {
                return false;
            }
            T x = radius_ - s.radius_;
            return (s.centre_ - centre_).r2() <= x * x;
        }

        template <std::floating_point T, std::size_t N>
        constexpr bool Sphere<T, N>::disjoint(const Sphere& s) const noexcept {
            T x = radius_ + s.radius_;
            return (s.centre_ - centre_).r2() >= x * x;
        }

        template <std::floating_point T, std::size_t N>
        T Sphere<T, N>::volume() const noexcept {

            using std::numbers::pi_v;

            if constexpr (N == 1) {

                return T{2} * radius_;

            } else if constexpr (N == 2) {

                return pi_v<T> * radius_ * radius_;

            } else if constexpr (N == 3) {

                static constexpr T c = pi_v<T> * T{4} / T{3};

                return c * radius_ * radius_ * radius_;

            } else {

                static const T log_pi = std::log(pi_v<T>);
                static const T a = static_cast<T>(N) / T{2};
                static const T b = a * log_pi - std::lgamma(a + T{1});
                static const T c = std::exp(b);

                return c * std::pow(radius_, static_cast<T>(N));

            }

        }

        template <std::floating_point T, std::size_t N>
        T Sphere<T, N>::surface() const noexcept {

            using std::numbers::pi_v;

            if constexpr (N == 1) {

                return T{2};

            } else if constexpr (N == 2) {

                return T{2} * pi_v<T> * radius_;

            } else if constexpr (N == 3) {

                return T{4} * pi_v<T> * radius_ * radius_;

            } else {

                static const T log_pi = std::log(pi_v<T>);
                static const T a = static_cast<T>(N) / T{2};
                static const T b = a * log_pi - std::lgamma(a);
                static const T c = T{2} * std::exp(b);

                return c * std::pow(radius_, static_cast<T>(N - 1));

            }

        }

    using Sphere_f2 = Sphere<float, 2>;
    using Sphere_f3 = Sphere<float, 3>;
    using Sphere_f4 = Sphere<float, 4>;
    using Sphere_d2 = Sphere<double, 2>;
    using Sphere_d3 = Sphere<double, 3>;
    using Sphere_d4 = Sphere<double, 4>;
    using Sphere_ld2 = Sphere<long double, 2>;
    using Sphere_ld3 = Sphere<long double, 3>;
    using Sphere_ld4 = Sphere<long double, 4>;

}

namespace std {

    template <RS::Scalar T, std::size_t N>
    struct std::formatter<RS::Graphics::Box<T, N>>:
    std::formatter<RS::Vector<T, N>> {
        template <typename FormatContext>
        auto format(const RS::Graphics::Box<T, N>& box, FormatContext& ctx) const {
            const std::formatter<RS::Vector<T, N>>& vector_format = *this;
            auto out = ctx.out();
            *out++ = 'B';
            out = vector_format.format(box.base(), ctx);
            *out++ = '+';
            out = vector_format.format(box.shape(), ctx);
            return out;
        }
    };

    template <std::floating_point T, std::size_t N>
    struct std::formatter<RS::Graphics::Sphere<T, N>>:
    std::formatter<T> {
        template <typename FormatContext>
        auto format(const RS::Graphics::Sphere<T, N>& sph, FormatContext& ctx) const {
            const std::formatter<T>& t_format = *this;
            auto out = ctx.out();
            *out++ = 'S';
            auto ch = '[';
            for (auto x: sph.centre()) {
                *out++ = ch;
                out = t_format.format(x, ctx);
                ch = ',';
            }
            *out++ = ']';
            *out++ = '+';
            out = t_format.format(sph.radius(), ctx);
            return out;
        }
    };

}
