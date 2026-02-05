#include "rs-graphics/colour.hpp"
#include "rs-core/linear-algebra.hpp"
#include "rs-core/unit-test.hpp"
#include "test/colour-space-samples.hpp"
#include <stdexcept>

using namespace RS;
using namespace RS::Graphics;
using namespace RS::Graphics::Test;

void test_rs_graphics_colour_hex_representation() {

    Rgb8 a;
    Rgba8 b;
    Rgbd c;
    Rgbad d;
    sRgb8 e;
    sRgba8 f;
    sRgbd g;
    sRgbad h;

    TRY(a = Rgb8("123456"));        TEST_EQUAL(a.R(), 0x12);  TEST_EQUAL(a.G(), 0x34);  TEST_EQUAL(a.B(), 0x56);  TEST_EQUAL(a.alpha(), 0xff);
    TRY(b = Rgba8("123456"));       TEST_EQUAL(b.R(), 0x12);  TEST_EQUAL(b.G(), 0x34);  TEST_EQUAL(b.B(), 0x56);  TEST_EQUAL(b.alpha(), 0xff);
    TRY(a = Rgb8("(789abcde)"));    TEST_EQUAL(a.R(), 0x78);  TEST_EQUAL(a.G(), 0x9a);  TEST_EQUAL(a.B(), 0xbc);  TEST_EQUAL(a.alpha(), 0xff);
    TRY(b = Rgba8("(789abcde)"));   TEST_EQUAL(b.R(), 0x78);  TEST_EQUAL(b.G(), 0x9a);  TEST_EQUAL(b.B(), 0xbc);  TEST_EQUAL(b.alpha(), 0xde);
    TRY(e = sRgb8("123456"));       TEST_EQUAL(e.R(), 0x12);  TEST_EQUAL(e.G(), 0x34);  TEST_EQUAL(e.B(), 0x56);  TEST_EQUAL(e.alpha(), 0xff);
    TRY(f = sRgba8("123456"));      TEST_EQUAL(f.R(), 0x12);  TEST_EQUAL(f.G(), 0x34);  TEST_EQUAL(f.B(), 0x56);  TEST_EQUAL(f.alpha(), 0xff);
    TRY(e = sRgb8("(789abcde)"));   TEST_EQUAL(e.R(), 0x78);  TEST_EQUAL(e.G(), 0x9a);  TEST_EQUAL(e.B(), 0xbc);  TEST_EQUAL(e.alpha(), 0xff);
    TRY(f = sRgba8("(789abcde)"));  TEST_EQUAL(f.R(), 0x78);  TEST_EQUAL(f.G(), 0x9a);  TEST_EQUAL(f.B(), 0xbc);  TEST_EQUAL(f.alpha(), 0xde);

    TRY(c = Rgbd("123456"));
    TEST_NEAR(c.R(), 0.070588, 1e-6);
    TEST_NEAR(c.G(), 0.203922, 1e-6);
    TEST_NEAR(c.B(), 0.337255, 1e-6);
    TEST_EQUAL(c.alpha(), 1);
    TRY(d = Rgbad("123456"));
    TEST_NEAR(d.R(), 0.070588, 1e-6);
    TEST_NEAR(d.G(), 0.203922, 1e-6);
    TEST_NEAR(d.B(), 0.337255, 1e-6);
    TEST_EQUAL(d.alpha(), 1);
    TRY(c = Rgbd("(789abcde)"));
    TEST_NEAR(c.R(), 0.470588, 1e-6);
    TEST_NEAR(c.G(), 0.603922, 1e-6);
    TEST_NEAR(c.B(), 0.737255, 1e-6);
    TEST_EQUAL(c.alpha(), 1);
    TRY(d = Rgbad("(789abcde)"));
    TEST_NEAR(d.R(), 0.470588, 1e-6);
    TEST_NEAR(d.G(), 0.603922, 1e-6);
    TEST_NEAR(d.B(), 0.737255, 1e-6);
    TEST_NEAR(d.alpha(), 0.870588, 1e-6);
    TRY(g = sRgbd("123456"));
    TEST_NEAR(g.R(), 0.070588, 1e-6);
    TEST_NEAR(g.G(), 0.203922, 1e-6);
    TEST_NEAR(g.B(), 0.337255, 1e-6);
    TEST_EQUAL(g.alpha(), 1);
    TRY(h = sRgbad("123456"));
    TEST_NEAR(h.R(), 0.070588, 1e-6);
    TEST_NEAR(h.G(), 0.203922, 1e-6);
    TEST_NEAR(h.B(), 0.337255, 1e-6);
    TEST_EQUAL(h.alpha(), 1);
    TRY(g = sRgbd("(789abcde)"));
    TEST_NEAR(g.R(), 0.470588, 1e-6);
    TEST_NEAR(g.G(), 0.603922, 1e-6);
    TEST_NEAR(g.B(), 0.737255, 1e-6);
    TEST_EQUAL(g.alpha(), 1);
    TRY(h = sRgbad("(789abcde)"));
    TEST_NEAR(h.R(), 0.470588, 1e-6);
    TEST_NEAR(h.G(), 0.603922, 1e-6);
    TEST_NEAR(h.B(), 0.737255, 1e-6);
    TEST_NEAR(h.alpha(), 0.870588, 1e-6);

    TEST_THROW_EXACT(Rgba8(""), std::invalid_argument, R"(Invalid colour: "")");
    TEST_THROW_EXACT(Rgba8("12345"), std::invalid_argument, R"(Invalid colour: "12345")");
    TEST_THROW_EXACT(Rgba8("1234567"), std::invalid_argument, R"(Invalid colour: "1234567")");
    TEST_THROW_EXACT(Rgba8("123456789"), std::invalid_argument, R"(Invalid colour: "123456789")");
    TEST_THROW_EXACT(Rgba8("abcdefgh"), std::invalid_argument, R"(Invalid colour: "abcdefgh")");

    TRY((a = Rgb8{0xfe, 0xdc, 0xba}));
    TRY((b = Rgba8{0x98, 0x76, 0x54, 0x32}));
    TRY((c = Rgbd{0.2, 0.4, 0.6}));
    TRY((d = Rgbad{0.8, 0.6, 0.4, 0.2}));
    TRY((e = sRgb8{0xfe, 0xdc, 0xba}));
    TRY((f = sRgba8{0x98, 0x76, 0x54, 0x32}));
    TRY((g = sRgbd{0.2, 0.4, 0.6}));
    TRY((h = sRgbad{0.8, 0.6, 0.4, 0.2}));

    TEST_EQUAL(a.hex(), "fedcba");
    TEST_EQUAL(b.hex(), "98765432");
    TEST_EQUAL(c.hex(), "336699");
    TEST_EQUAL(d.hex(), "cc996633");
    TEST_EQUAL(e.hex(), "fedcba");
    TEST_EQUAL(f.hex(), "98765432");
    TEST_EQUAL(g.hex(), "336699");
    TEST_EQUAL(h.hex(), "cc996633");

}
