#pragma once

// This header is not part of the public interface

#include "rs-core/arithmetic.hpp"
#include <cmath>
#include <concepts>
#include <cstdlib>

namespace RS::Graphics::Detail {

    template <std::floating_point T, std::invocable<T> F, std::invocable<T> DF>
    requires requires (T t, F f, DF df) {
        { f(t) } -> std::convertible_to<T>;
        { df(t) } -> std::convertible_to<T>;
    }
    struct NewtonRaphson {

        static constexpr T default_epsilon = (sizeof(T) < sizeof(double)) ? static_cast<T>(1e-5) : static_cast<T>(1e-12);
        static constexpr int default_max_iterations = 100;

        F function;
        DF derivative;
        T epsilon;
        int max_iterations;
        T error;
        int iterations;

        explicit NewtonRaphson(F f, DF df, T eps = default_epsilon, int max_iter = default_max_iterations):
        function(f), derivative(df), epsilon(eps), max_iterations(max_iter) {}

        T solve(T x = 0, T y = 0) {

            error = 0;
            iterations = 0;

            while (iterations < max_iterations) {

                ++iterations;
                auto y1 = function(x) - y;
                error = std::abs(y1);

                if (error <= epsilon) {
                    break;
                }

                T slope = derivative(x);

                if (slope != T{0}) {
                    x -= y1 / slope;
                } else if ((iterations & 1) == 0) {
                    x += T{100} * epsilon;
                } else {
                    x -= T{100} * epsilon;
                }

            }

            return x;

        }

    };

    template <std::floating_point T, std::invocable<T> F, std::invocable<T> DF>
    requires requires (T t, F f, DF df) {
        { f(t) } -> std::convertible_to<T>;
        { df(t) } -> std::convertible_to<T>;
    }
    auto newton_raphson(F f, DF df) {
        return NewtonRaphson<T, F, DF>{f, df};
    }

}
