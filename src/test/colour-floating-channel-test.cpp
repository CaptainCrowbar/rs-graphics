#include "rs-graphics/colour.hpp"
#include "rs-core/linear-algebra.hpp"
#include "rs-core/unit-test.hpp"
#include "test/colour-space-samples.hpp"
#include <format>

using namespace RS;
using namespace RS::Graphics;
using namespace RS::Graphics::Test;

void test_rs_graphics_colour_floating_point_elements() {

    Rgbad c;

    TEST_EQUAL(std::format("{}", c), "[0,0,0,0]");

    TEST_EQUAL(&c.R() - c.begin(), 0);
    TEST_EQUAL(&c.G() - c.begin(), 1);
    TEST_EQUAL(&c.B() - c.begin(), 2);
    TEST_EQUAL(&c.alpha() - c.begin(), 3);

    TEST_EQUAL(c.R(), 0);      TRY(c.R() = 0.25);   TEST_EQUAL(c.R(), 0.25);
    TEST_EQUAL(c.G(), 0);      TRY(c.G() = 0.5);    TEST_EQUAL(c.G(), 0.5);
    TEST_EQUAL(c.B(), 0);      TRY(c.B() = 0.75);   TEST_EQUAL(c.B(), 0.75);
    TEST_EQUAL(c.alpha(), 0);  TRY(c.alpha() = 1);  TEST_EQUAL(c.alpha(), 1);

    TEST_RANGES_NEAR(c.as_vector(), Double4(0.25,0.5,0.75,1), 1e-15);
    TEST_RANGES_NEAR(c, Double4(0.25,0.5,0.75,1), 1e-15);
    TEST_EQUAL(std::format("{}", c), "[0.25,0.5,0.75,1]");

    TRY((c = Rgbad{0.2,0.4,0.6}));      TEST_EQUAL(std::format("{}", c), "[0.2,0.4,0.6,1]");
    TRY((c = Rgbad{0.2,0.4,0.6,0.8}));  TEST_EQUAL(std::format("{}", c), "[0.2,0.4,0.6,0.8]");

}

void test_rs_graphics_colour_floating_point_arithmetic() {

    Rgbad a{0.1,0.2,0.3,0.4};
    Rgbad b{0.8,0.6,0.4,0.2};
    Rgbad c;
    Double4 d{2,3,4,5};

    TRY(c = + a);    TEST_RANGES_NEAR(c, Double4(0.1,0.2,0.3,0.4), 1e-10);
    TRY(c = - a);    TEST_RANGES_NEAR(c, Double4(-0.1,-0.2,-0.3,-0.4), 1e-10);
    TRY(c = a + b);  TEST_RANGES_NEAR(c, Double4(0.9,0.8,0.7,0.6), 1e-10);
    TRY(c = a - b);  TEST_RANGES_NEAR(c, Double4(-0.7,-0.4,-0.1,0.2), 1e-10);
    TRY(c = a * 2);  TEST_RANGES_NEAR(c, Double4(0.2,0.4,0.6,0.8), 1e-10);
    TRY(c = 2 * a);  TEST_RANGES_NEAR(c, Double4(0.2,0.4,0.6,0.8), 1e-10);
    TRY(c = a / 2);  TEST_RANGES_NEAR(c, Double4(0.05,0.1,0.15,0.2), 1e-10);
    TRY(c = a * d);  TEST_RANGES_NEAR(c, Double4(0.2,0.6,1.2,2), 1e-10);
    TRY(c = d * a);  TEST_RANGES_NEAR(c, Double4(0.2,0.6,1.2,2), 1e-10);
    TRY(c = b / d);  TEST_RANGES_NEAR(c, Double4(0.4,0.2,0.1,0.04), 1e-10);

}

void test_rs_graphics_colour_floating_point_standard_colours() {

    TEST_EQUAL(std::format("{}", Rgbad::clear()),    "[0,0,0,0]");
    TEST_EQUAL(std::format("{}", Rgbad::black()),    "[0,0,0,1]");
    TEST_EQUAL(std::format("{}", Rgbad::white()),    "[1,1,1,1]");
    TEST_EQUAL(std::format("{}", Rgbad::red()),      "[1,0,0,1]");
    TEST_EQUAL(std::format("{}", Rgbad::yellow()),   "[1,1,0,1]");
    TEST_EQUAL(std::format("{}", Rgbad::green()),    "[0,1,0,1]");
    TEST_EQUAL(std::format("{}", Rgbad::cyan()),     "[0,1,1,1]");
    TEST_EQUAL(std::format("{}", Rgbad::blue()),     "[0,0,1,1]");
    TEST_EQUAL(std::format("{}", Rgbad::magenta()),  "[1,0,1,1]");

}
