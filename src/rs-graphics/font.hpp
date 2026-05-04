#pragma once

#include "rs-graphics/colour.hpp"
#include "rs-graphics/geometry.hpp"
#include "rs-graphics/image.hpp"
#include "rs-graphics/image-mask.hpp"
#include "rs-core/enum.hpp"
#include "rs-core/linear-algebra.hpp"
#include "rs-unicode/encoding.hpp"
#include "rs-unicode/string.hpp"
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace RS::Graphics {

    namespace Detail {

        template <ColourType C>
        C default_text_background() noexcept {
            return C::has_alpha ? C::clear() : C::white();
        }

    }

    RS_BITMASK(FontStyle, unsigned char,
        regular   = 0,
        bold      = 1,
        italic    = 2,
        fallback  = 4,
    )

    class Font {

    public:

        Font() = default;
        explicit Font(const std::filesystem::path& file, int index = 0);
        virtual ~Font() noexcept {}

        explicit operator bool() const noexcept { return static_cast<bool>(font_); }

        std::string family() const;
        std::string subfamily() const;
        std::string name() const;
        bool has_glyph(char32_t c) const noexcept;
        bool has_glyphs(char32_t first, char32_t last) const noexcept;
        template <ReadableRange<char32_t> Range> bool has_glyphs(const Range& range) const;

        static std::vector<Font> load(const std::filesystem::path& file);

    protected:

        struct font_impl;

        std::shared_ptr<std::string> content_;
        std::shared_ptr<font_impl> font_;

    private:

        bool has_glyph_unchecked(char32_t c) const noexcept;

    };

        template <ReadableRange<char32_t> Range>
        bool Font::has_glyphs(const Range& range) const {
            if (! font_) {
                return false;
            }
            for (auto c: range) {
                if (! has_glyph_unchecked(c)) {
                    return false;
                }
            }
            return true;
        }

    class ScaledFont:
    public Font {

    public:

        ScaledFont() = default;
        ScaledFont(const Font& font, int scale) noexcept: ScaledFont{font, {scale, scale}} {}
        ScaledFont(const Font& font, Int2 scale) noexcept;

        Int2 scale() const noexcept;
        int ascent() const noexcept;
        int descent() const noexcept;
        int line_gap() const noexcept;
        int line_offset() const noexcept { return ascent() - descent() + line_gap(); }
        template <LinearColourType C, ImageFlags F>
            void render(Image<C, F>& image, Int2& offset, const std::string& text, int line_shift = 0,
                C text_colour = C::black(), C background = Detail::default_text_background<C>()) const;
        template <LinearColourType C, ImageFlags F>
            void render_to(Image<C, F>& image, Int2 ref_point, const std::string& text, int line_shift = 0,
                C text_colour = C::black()) const;
        Box_i2 text_box(const std::string& text, int line_shift = 0) const;
        std::size_t text_fit(const std::string& text, std::size_t max_pixels) const;
        std::size_t text_wrap(const std::string& text_in, std::string& text_out, std::size_t max_pixels) const;

    private:

        static constexpr float byte_scale = 1.0f / 255.0f;

        struct scaled_impl;

        std::shared_ptr<scaled_impl> scaled_;

        Detail::ByteMask render_glyph_mask(char32_t c, Int2& offset) const;
        Detail::ByteMask render_text_mask(const std::u32string& utext, int line_shift, Int2& offset) const;
        int scale_x(int x) const noexcept;
        int scale_y(int y) const noexcept;
        Box_i2 scale_box(Box_i2 box) const noexcept;

    };

        template <LinearColourType C, ImageFlags F>
        void ScaledFont::render(Image<C, F>& image, Int2& offset, const std::string& text, int line_shift,
                C text_colour, C background) const {

            using namespace RS::Unicode;

            if (! font_) {
                throw std::invalid_argument("No font");
            }

            image.clear();
            offset = Int2::null();

            if (text.empty()) {
                return;
            }

            auto utext = utf8_to_utf32(text, Convert::checked);
            auto mask = render_text_mask(utext, line_shift, offset);

            if (! mask.empty()) {
                mask.make_image(image, text_colour, background);
            }

        }

        template <LinearColourType C, ImageFlags F>
        void ScaledFont::render_to(Image<C, F>& image, Int2 ref_point, const std::string& text, int line_shift,
                C text_colour) const {

            using namespace RS::Unicode;

            if (! font_) {
                throw std::invalid_argument("No font");
            } else if (text.empty()) {
                return;
            }

            auto utext = utf8_to_utf32(text, Convert::checked);
            Int2 offset;
            auto mask = render_text_mask(utext, line_shift, offset);

            if (mask.empty()) {
                return;
            }

            mask.onto_image(image, ref_point + offset, text_colour);

        }

    class FontMap {

    public:

        void clear() noexcept { table_.clear(); }
        bool contains(const std::string& family) const noexcept { return table_.count(family) != 0; }
        bool contains(const std::string& family, const std::string& subfamily) const noexcept;
        bool empty() const noexcept { return table_.empty(); }
        std::vector<std::string> families() const;
        std::vector<std::string> subfamilies(const std::string& family) const;
        Font find(const std::vector<std::string>& families, FontStyle style = FontStyle::regular) const;
        Font load(const std::string& family, const std::string& subfamily) const;
        void search(const std::filesystem::path& dir, bool recurse = false);
        void search_system();
        std::size_t size() const noexcept { return table_.size(); }

    private:

        using compare_icase = RS::Unicode::BasicStringCompare<RS::Unicode::Sort::icase>;

        struct mapped_type {
            std::filesystem::path file;
            int index;
        };

        using inner_table = std::map<std::string, mapped_type, compare_icase>;
        using outer_table = std::map<std::string, inner_table, compare_icase>;

        outer_table table_;

    };

}
