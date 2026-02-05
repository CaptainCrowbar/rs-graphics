#include "rs-graphics/colour.hpp"
#include "rs-core/linear-algebra.hpp"
#include "rs-core/unit-test.hpp"
#include "test/colour-space-samples.hpp"

using namespace RS;
using namespace RS::Graphics;
using namespace RS::Graphics::Test;

void test_rs_graphics_colour_concepts() {

    using C1 = Colour<float, LinearRGB>;
    using C2 = Colour<float, sRGB>;

    TEST(ColourType<C1>);
    TEST(ColourType<C2>);
    TEST(! ColourType<int>);
    TEST(LinearColourType<C1>);
    TEST(! LinearColourType<C2>);
    TEST(! LinearColourType<int>);

}

void test_rs_graphics_colour_channel_order() {

    Colour<double, LinearRGB, ColourLayout::forward> a;
    Colour<double, LinearRGB, ColourLayout::forward_alpha> b;
    Colour<double, LinearRGB, ColourLayout::alpha_forward> c;
    Colour<double, LinearRGB, ColourLayout::reverse> d;
    Colour<double, LinearRGB, ColourLayout::reverse_alpha> e;
    Colour<double, LinearRGB, ColourLayout::alpha_reverse> f;

    a.R() = 0.1;  a.G() = 0.2;  a.B() = 0.3;
    b.R() = 0.1;  b.G() = 0.2;  b.B() = 0.3;  b.alpha() = 0.4;
    c.R() = 0.1;  c.G() = 0.2;  c.B() = 0.3;  c.alpha() = 0.4;
    d.R() = 0.1;  d.G() = 0.2;  d.B() = 0.3;
    e.R() = 0.1;  e.G() = 0.2;  e.B() = 0.3;  e.alpha() = 0.4;
    f.R() = 0.1;  f.G() = 0.2;  f.B() = 0.3;  f.alpha() = 0.4;

    TEST_RANGES_NEAR(a, Double3(0.1,0.2,0.3), 1e-10);
    TEST_RANGES_NEAR(b, Double4(0.1,0.2,0.3,0.4), 1e-10);
    TEST_RANGES_NEAR(c, Double4(0.4,0.1,0.2,0.3), 1e-10);
    TEST_RANGES_NEAR(d, Double3(0.3,0.2,0.1), 1e-10);
    TEST_RANGES_NEAR(e, Double4(0.3,0.2,0.1,0.4), 1e-10);
    TEST_RANGES_NEAR(f, Double4(0.4,0.3,0.2,0.1), 1e-10);

    a.cs(0) = 0.01;  a.cs(1) = 0.02;  a.cs(2) = 0.03;
    b.cs(0) = 0.01;  b.cs(1) = 0.02;  b.cs(2) = 0.03;  b.cs(3) = 0.04;
    c.cs(0) = 0.01;  c.cs(1) = 0.02;  c.cs(2) = 0.03;  c.cs(3) = 0.04;
    d.cs(0) = 0.01;  d.cs(1) = 0.02;  d.cs(2) = 0.03;
    e.cs(0) = 0.01;  e.cs(1) = 0.02;  e.cs(2) = 0.03;  e.cs(3) = 0.04;
    f.cs(0) = 0.01;  f.cs(1) = 0.02;  f.cs(2) = 0.03;  f.cs(3) = 0.04;

    TEST_RANGES_NEAR(a, Double3(0.01,0.02,0.03), 1e-10);
    TEST_RANGES_NEAR(b, Double4(0.01,0.02,0.03,0.04), 1e-10);
    TEST_RANGES_NEAR(c, Double4(0.04,0.01,0.02,0.03), 1e-10);
    TEST_RANGES_NEAR(d, Double3(0.03,0.02,0.01), 1e-10);
    TEST_RANGES_NEAR(e, Double4(0.03,0.02,0.01,0.04), 1e-10);
    TEST_RANGES_NEAR(f, Double4(0.04,0.03,0.02,0.01), 1e-10);

}

void test_rs_graphics_colour_premultiplied_alpha() {

    Rgba8 bc1, bc2, bc3;
    Rgbad fc1, fc2, fc3;

    TRY((bc1 = Rgba8{50,100,150,200}));
    TRY((fc1 = Rgbad{0.2,0.4,0.6,0.8}));

    TRY(bc2 = bc1.multiply_alpha());
    TRY(fc2 = fc1.multiply_alpha());

    TEST_RANGES_NEAR(bc2, Double4(39,78,118,200), 0);
    TEST_RANGES_NEAR(fc2, Double4(0.16,0.32,0.48,0.8), 1e-10);

    TRY(bc3 = bc2.unmultiply_alpha());
    TRY(fc3 = fc2.unmultiply_alpha());

    TEST_RANGES_NEAR(bc3, bc1, 1);
    TEST_RANGES_NEAR(fc3, fc1, 1e-10);

}

void test_rs_graphics_colour_alpha_blending() {

    Rgbad a, b, c, pa, pb, pc;
    Rgba8 d, e, f, pd, pe, pf;

    TRY((a = Rgbad{0.2,0.4,0.6,0.8}));
    TRY((b = Rgbad{0.9,0.8,0.7,0.6}));
    TRY(c = alpha_blend(a, b));
    TEST_RANGES_NEAR(c, Double4(0.291304,0.452174,0.613043,0.92), 1e-6);

    TRY(pa = a.multiply_alpha());
    TRY(pb = b.multiply_alpha());
    TRY(pc = alpha_blend(pa, pb, Pma::all));
    TRY(c = pc.unmultiply_alpha());
    TEST_RANGES_NEAR(c, Double4(0.291304,0.452174,0.613043,0.92), 1e-6);

    TRY((d = Rgba8{50,100,150,200}));
    TRY((e = Rgba8{250,225,200,175}));
    TRY(f = alpha_blend(d, e));
    TEST_RANGES_NEAR(f, Double4(82,120,158,238), 0);

    TRY(pd = d.multiply_alpha());
    TRY(pe = e.multiply_alpha());
    TRY(pf = alpha_blend(pd, pe, Pma::all));
    TRY(f = pf.unmultiply_alpha());
    TEST_RANGES_NEAR(f, Double4(82,120,158,238), 1);

}
