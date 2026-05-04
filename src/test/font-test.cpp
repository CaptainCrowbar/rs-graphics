#include "rs-graphics/font.hpp"
#include "rs-graphics/colour.hpp"
#include "rs-graphics/geometry.hpp"
#include "rs-graphics/image.hpp"
#include "rs-core/linear-algebra.hpp"
#include "rs-core/unit-test.hpp"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace RS;
using namespace RS::Graphics;
using namespace std::literals;

namespace {

    const fs::path fonts_dir = "../src/test/fonts";
    const fs::path mono_file = fonts_dir / "DejaVuSansMono.ttf";
    const fs::path serif_file = fonts_dir / "DejaVuSerif.ttf";
    const fs::path ttc_file = fonts_dir / "DejaVu2.ttc";
    const fs::path no_such_file = fonts_dir / "NoSuchFile.ttf";

}

void test_rs_graphics_font_loading() {

    Font font;
    std::vector<Font> fonts;

    TRY(font = Font(mono_file));
    TEST(font);
    TRY(fonts = Font::load(mono_file));
    TEST_EQUAL(fonts.size(), 1u);

    TRY(font = Font(serif_file));
    TEST(font);
    TRY(fonts = Font::load(serif_file));
    TEST_EQUAL(fonts.size(), 1u);

    TRY(fonts = Font::load(ttc_file));
    TEST_EQUAL(fonts.size(), 2u);

    TRY(font = Font(no_such_file));
    TEST(! font);

}

void test_rs_graphics_font_properties() {

    Font mono, serif, none;
    std::vector<Font> ttc;
    std::u32string uchars = U"ABCDE";

    TRY(mono = Font(mono_file));
    TEST_EQUAL(mono.family(), "DejaVu Sans Mono");
    TEST_EQUAL(mono.subfamily(), "Book");
    TEST_EQUAL(mono.name(), "DejaVu Sans Mono");
    TEST(! mono.has_glyph(0));
    TEST(mono.has_glyph(U'A'));
    TEST(mono.has_glyph(0xfffd));
    TEST(! mono.has_glyph(0xffff));
    TEST(mono.has_glyphs(0x20, 0x7e));
    TEST(mono.has_glyphs(uchars));

    TRY(serif = Font(serif_file));
    TEST_EQUAL(serif.family(), "DejaVu Serif");
    TEST_EQUAL(serif.subfamily(), "Book");
    TEST_EQUAL(serif.name(), "DejaVu Serif");
    TEST(! serif.has_glyph(0));
    TEST(serif.has_glyph(U'A'));
    TEST(serif.has_glyph(0xfffd));
    TEST(! serif.has_glyph(0xffff));
    TEST(serif.has_glyphs(0x20, 0x7e));
    TEST(serif.has_glyphs(uchars));

    TRY(none = Font(no_such_file));
    TEST_EQUAL(none.family(), "");
    TEST_EQUAL(none.subfamily(), "");
    TEST_EQUAL(none.name(), "");
    TEST(! none.has_glyph(0));
    TEST(! none.has_glyph(U'A'));
    TEST(! none.has_glyph(0xfffd));
    TEST(! none.has_glyph(0xffff));
    TEST(! none.has_glyphs(0x20, 0x7e));
    TEST(! none.has_glyphs(uchars));

    TRY(ttc = Font::load(ttc_file));
    TEST_EQUAL(ttc.size(), 2u);

    for (auto& font: ttc) {
        TEST(! font.has_glyph(0));
        TEST(font.has_glyph(U'A'));
        TEST(font.has_glyph(0xfffd));
        TEST(! font.has_glyph(0xffff));
        TEST(font.has_glyphs(0x20, 0x7e));
        TEST(font.has_glyphs(uchars));
    }

}

void test_rs_graphics_font_metrics() {

    Font mono, serif;
    ScaledFont s_mono, s_serif;

    TRY(mono = Font(mono_file));
    TRY(s_mono = ScaledFont(mono, 100));
    TEST_EQUAL(s_mono.scale(), Int2(100, 100));
    TEST_EQUAL(s_mono.ascent(), 93);
    TEST_EQUAL(s_mono.descent(), -24);
    TEST_EQUAL(s_mono.line_gap(), 0);
    TEST_EQUAL(s_mono.line_offset(), 117);

    TRY(serif = Font(serif_file));
    TRY(s_serif = ScaledFont(serif, 100));
    TEST_EQUAL(s_serif.scale(), Int2(100, 100));
    TEST_EQUAL(s_serif.ascent(), 93);
    TEST_EQUAL(s_serif.descent(), -24);
    TEST_EQUAL(s_serif.line_gap(), 0);
    TEST_EQUAL(s_serif.line_offset(), 117);

}

void test_rs_graphics_font_text_metrics() {

    auto text = "Hello world\nGoodbye"s;
    Font mono, serif;
    ScaledFont s_mono, s_serif;
    Box_i2 box;

    TRY(mono = Font(mono_file));
    TRY(s_mono = ScaledFont(mono, 100));
    TRY(box = s_mono.text_box(text, 0));
    TEST_EQUAL(box, Box_i2({0,-77}, {651, 215}));

    TRY(serif = Font(serif_file));
    TRY(s_serif = ScaledFont(serif, 100));
    TRY(box = s_serif.text_box(text, 0));
    TEST_EQUAL(box, Box_i2({0,-76}, {590, 216}));

}

void test_rs_graphics_font_text_fitting() {

    auto text = "Hello world"s;
    Font mono, serif;
    ScaledFont s_mono, s_serif;
    std::size_t length;

    TRY(mono = Font(mono_file));
    TRY(s_mono = ScaledFont(mono, 100));

    TRY(length = s_mono.text_fit(text, 200));  TEST_EQUAL(length, 3u);
    TRY(length = s_mono.text_fit(text, 400));  TEST_EQUAL(length, 6u);
    TRY(length = s_mono.text_fit(text, 600));  TEST_EQUAL(length, 10u);
    TRY(length = s_mono.text_fit(text, 800));  TEST_EQUAL(length, npos);

    TRY(serif = Font(serif_file));
    TRY(s_serif = ScaledFont(serif, 100));

    TRY(length = s_serif.text_fit(text, 200));  TEST_EQUAL(length, 3u);
    TRY(length = s_serif.text_fit(text, 400));  TEST_EQUAL(length, 7u);
    TRY(length = s_serif.text_fit(text, 600));  TEST_EQUAL(length, npos);

}

void test_rs_graphics_font_text_wrapping() {

    auto lorem =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n"
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n"s;

    Font mono, serif;
    ScaledFont s_mono, s_serif;
    std::string result;
    std::size_t lines;

    TRY(mono = Font(mono_file));
    TRY(serif = Font(serif_file));
    TRY(s_mono = ScaledFont(mono, 100));
    TRY(s_serif = ScaledFont(serif, 100));

    TRY(lines = s_mono.text_wrap("", result, 1000));
    TEST_EQUAL(lines, 0u);

    TRY(lines = s_mono.text_wrap(lorem, result, 1000));
    TEST_EQUAL(lines, 32u);

    TEST_EQUAL(result,
        "Lorem ipsum dolor\n"
        "sit amet,\n"
        "consectetur\n"
        "adipiscing elit,\n"
        "sed do eiusmod\n"
        "tempor incididunt\n"
        "ut labore et\n"
        "dolore magna\n"
        "aliqua.\n"
        "Ut enim ad minim\n"
        "veniam, quis\n"
        "nostrud\n"
        "exercitation\n"
        "ullamco laboris\n"
        "nisi ut aliquip\n"
        "ex ea commodo\n"
        "consequat.\n"
        "Duis aute irure\n"
        "dolor in\n"
        "reprehenderit in\n"
        "voluptate velit\n"
        "esse cillum\n"
        "dolore eu fugiat\n"
        "nulla pariatur.\n"
        "Excepteur sint\n"
        "occaecat\n"
        "cupidatat non\n"
        "proident, sunt in\n"
        "culpa qui officia\n"
        "deserunt mollit\n"
        "anim id est\n"
        "laborum."
    );

    TRY(lines = s_mono.text_wrap(lorem, result, 5000));
    TEST_EQUAL(lines, 8u);

    TEST_EQUAL(result,
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor\n"
        "incididunt ut labore et dolore magna aliqua.\n"
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip\n"
        "ex ea commodo consequat.\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu\n"
        "fugiat nulla pariatur.\n"
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt\n"
        "mollit anim id est laborum."
    );

    TRY(lines = s_serif.text_wrap("", result, 1000));
    TEST_EQUAL(lines, 0u);

    TRY(lines = s_serif.text_wrap(lorem, result, 1000));
    TEST_EQUAL(lines, 28u);

    TEST_EQUAL(result,
        "Lorem ipsum dolor\n"
        "sit amet,\n"
        "consectetur\n"
        "adipiscing elit, sed\n"
        "do eiusmod tempor\n"
        "incididunt ut labore\n"
        "et dolore magna\n"
        "aliqua.\n"
        "Ut enim ad minim\n"
        "veniam, quis nostrud\n"
        "exercitation ullamco\n"
        "laboris nisi ut\n"
        "aliquip ex ea\n"
        "commodo\n"
        "consequat.\n"
        "Duis aute irure\n"
        "dolor in\n"
        "reprehenderit in\n"
        "voluptate velit esse\n"
        "cillum dolore eu\n"
        "fugiat nulla\n"
        "pariatur.\n"
        "Excepteur sint\n"
        "occaecat cupidatat\n"
        "non proident, sunt in\n"
        "culpa qui officia\n"
        "deserunt mollit anim\n"
        "id est laborum."
    );

    TRY(lines = s_serif.text_wrap(lorem, result, 5000));
    TEST_EQUAL(lines, 7u);

    TEST_EQUAL(result,
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut\n"
        "labore et dolore magna aliqua.\n"
        "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo\n"
        "consequat.\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n"
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id\n"
        "est laborum."
    );

}

void test_rs_graphics_font_rendering() {

    auto text = "Hello world\nGoodbye"s;
    Font mono, serif;
    ScaledFont s_mono, s_serif;
    Image<Rgbaf> image;
    Int2 offset;
    std::size_t n_empty, n_partial, n_full;

    auto take_census = [&] () {
        n_empty = n_partial = n_full = 0;
        for (auto& pixel: image) {
            if (pixel.alpha() == 0 || pixel == Rgbaf::white()) {
                ++n_empty;
            } else if (pixel == Rgbaf::blue()) {
                ++n_full;
            } else {
                ++n_partial;
            }
        }
    };

    TRY(mono = Font(mono_file));
    TRY(s_mono = ScaledFont(mono, 100));
    TRY(s_mono.render(image, offset, text, 0, Rgbaf::blue()));
    TEST_EQUAL(offset, Int2(0, -77));
    TEST_EQUAL(image.shape(), Int2(651, 215));
    TRY(image.save("mono1.png"));

    TRY(take_census());
    TEST_EQUAL(n_empty, 115'908u);
    TEST_EQUAL(n_partial, 5'565u);
    TEST_EQUAL(n_full, 18'492u);
    TEST_EQUAL(n_empty + n_partial + n_full, image.size());

    TRY(serif = Font(serif_file));
    TRY(s_serif = ScaledFont(serif, 100));
    TRY(s_serif.render(image, offset, text, 0, Rgbaf::blue()));
    TEST_EQUAL(offset, Int2(0, -76));
    TEST_EQUAL(image.shape(), Int2(590, 216));
    TRY(image.save("serif1.png"));

    TRY(take_census());
    TEST_EQUAL(n_empty, 103'401u);
    TEST_EQUAL(n_partial, 6'046u);
    TEST_EQUAL(n_full, 17'993u);
    TEST_EQUAL(n_empty + n_partial + n_full, image.size());

    TRY(image.reset({1000, 500}, Rgbaf::white()));
    TRY(s_mono.render_to(image, {100, 100}, text, 0, Rgbaf::blue()));
    TRY(image.save("mono2.png"));

    TRY(take_census());
    TEST_EQUAL(n_empty, 475'943u);
    TEST_EQUAL(n_partial, 5'565u);
    TEST_EQUAL(n_full, 18'492u);
    TEST_EQUAL(n_empty + n_partial + n_full, image.size());

    TRY(image.reset({1000, 500}, Rgbaf::white()));
    TRY(s_serif.render_to(image, {100, 100}, text, 0, Rgbaf::blue()));
    TRY(image.save("serif2.png"));

    TRY(take_census());
    TEST_EQUAL(n_empty, 475'961u);
    TEST_EQUAL(n_partial, 6'046u);
    TEST_EQUAL(n_full, 17'993u);
    TEST_EQUAL(n_empty + n_partial + n_full, image.size());

}

void test_rs_graphics_font_map() {

    FontMap map;
    Font font;
    std::vector<std::string> families;

    TRY(map.search_system());
    TEST(! map.empty());
    TRY(families = map.families());
    TEST_EQUAL(families.size(), map.size());

    TRY(font = map.find({"Helvetica", "Arial", "DejaVu Sans"}));
    TEST(font);
    TEST_MATCH(font.name(), "^(Helvetica|Arial|DejaVu Sans)$");
    TRY(font = map.find({"Helvetica", "Arial", "DejaVu Sans"}, FontStyle::bold));
    TEST(font);
    TEST_MATCH(font.name(), "^(Helvetica|Arial|DejaVu Sans) Bold$");

}
