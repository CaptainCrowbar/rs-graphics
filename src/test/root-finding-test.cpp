#include "rs-graphics/root-finding.hpp"
#include "rs-core/unit-test.hpp"
#include <cmath>

using namespace RS;
using namespace RS::Graphics;
using namespace RS::Graphics::Detail;

void test_rs_graphics_root_finding_newton_raphson_float() {

    auto k = 0.0f;
    auto x = 0.0f;
    auto root1 = 0.865'474f;

    auto f = [&k] (float x) { return std::cos(k * x) - std::pow(k * x, 3.0f); };
    auto df = [&k] (float x) { return - k * (std::sin(k * x) + 3.0f * std::pow(k * x, 2.0f)); };

    auto nr = newton_raphson<float>(f, df);

    for (auto i = 1; i <= 1024; i *= 2) {
        k = static_cast<float>(i);
        TRY(x = nr.solve(1.0f, 0.0f));
        TEST_NEAR(x, root1 / i, 1e-5f / i);
        TEST_NEAR(nr.error, 0.0f, 1e-5f);
        TEST(nr.iterations < 25);
    }

    for (auto i = 2; i <= 1024; i *= 2) {
        k = 1.0f / static_cast<float>(i);
        TRY(x = nr.solve(1.0f, 0.0f));
        TEST_NEAR(x, root1 * i, 1e-5f * i);
        TEST_NEAR(nr.error, 0.0f, 1e-5f);
        TEST(nr.iterations < 25);
    }

}

void test_rs_graphics_root_finding_newton_raphson_double() {

    auto k = 0.0;
    auto x = 0.0;
    auto root1 = 0.865'474'033'1;

    auto f = [&k] (double x) { return std::cos(k * x) - std::pow(k * x, 3.0); };
    auto df = [&k] (double x) { return - k * (std::sin(k * x) + 3.0 * std::pow(k * x, 2.0)); };

    auto nr = newton_raphson<double>(f, df);

    for (auto i = 1; i <= 1024; i *= 2) {
        k = static_cast<double>(i);
        TRY(x = nr.solve(1.0, 0.0));
        TEST_NEAR(x, root1 / i, 1e-10 / i);
        TEST_NEAR(nr.error, 0.0, 1e-10);
        TEST(nr.iterations < 25);
    }

    for (auto i = 2; i <= 1024; i *= 2) {
        k = 1.0 / static_cast<double>(i);
        TRY(x = nr.solve(1.0, 0.0));
        TEST_NEAR(x, root1 * i, 1e-10 * i);
        TEST_NEAR(nr.error, 0.0, 1e-10);
        TEST(nr.iterations < 25);
    }

}
