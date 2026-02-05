#pragma once

#include "rs-graphics/colour.hpp"
#include "rs-graphics/colour-space.hpp"
#include "rs-graphics/geometry.hpp"
#include "rs-core/character.hpp"
#include "rs-core/enum.hpp"
#include "rs-core/linear-algebra.hpp"
#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <new>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace RS::Graphics {

    RS_BITMASK(ImageFlags, std::uint8_t,
        none    = 0,
        invert  = 1,
        pma     = 2,
    )

    RS_BITMASK(ImageResize, std::uint8_t,
        none    = 0,
        unlock  = 1,
        wrap    = 2,
    )

    class ImageIoError:
    public std::runtime_error {
    public:
        ImageIoError(const std::filesystem::path& file, const std::string& details, bool stbi):
            std::runtime_error{make_message(file, details, stbi)}, file_{file} {}
        std::filesystem::path file() const noexcept { return file_; }
    private:
        std::filesystem::path file_;
        static std::string make_message(const std::filesystem::path& file, const std::string& details, bool stbi);
    };

    struct ImageInfo {
        Int2 shape;
        int channels = 0;
        int bits_per_channel = 0;
        bool has_alpha = false;
        bool is_hdr = false;
        explicit operator bool() const noexcept { return ! shape.is_null(); }
        std::string str() const;
    };

    ImageInfo query_image(const std::filesystem::path& file) noexcept;

    template <ColourType CT = Rgbaf, ImageFlags Flags = ImageFlags::none> class Image;

    namespace Detail {

        struct FreeMem {
            void operator()(void* ptr) const noexcept {
                if (ptr != nullptr) {
                    std::free(ptr);
                }
            }
        };

        template <typename T> using StbiPtr = std::unique_ptr<T, FreeMem>;

        StbiPtr<std::uint8_t> load_image_8(const std::filesystem::path& file, Int2& shape);
        StbiPtr<std::uint16_t> load_image_16(const std::filesystem::path& file, Int2& shape);
        StbiPtr<float> load_image_hdr(const std::filesystem::path& file, Int2& shape);
        void save_image_8(const Image<Rgba8>& image, const std::filesystem::path& file, const std::string& format, int quality);
        void save_image_hdr(const Image<Rgbaf>& image, const std::filesystem::path& file);
        void resize_image_8(const std::uint8_t* in, Int2 ishape, std::uint8_t* out, Int2 oshape, int num_channels, int alpha_channel,
            int stb_flags, int stb_edge, int stb_filter, int stb_space);
        void resize_image_16(const std::uint16_t* in, Int2 ishape, std::uint16_t* out, Int2 oshape, int num_channels, int alpha_channel,
            int stb_flags, int stb_edge, int stb_filter, int stb_space);
        void resize_image_hdr(const float* in, Int2 ishape, float* out, Int2 oshape, int num_channels, int alpha_channel,
            int stb_flags, int stb_edge, int stb_filter, int stb_space);

        template <typename Img, typename Clr, bool Const>
        class ImageIterator {

        private:

            using CI = std::conditional_t<Const, const Img, Img>;
            using CC = std::conditional_t<Const, const Clr, Clr>;

            CI* image_;
            std::int64_t index_;

        public:

            using difference_type = std::int64_t;
            using iterator_category = std::bidirectional_iterator_tag;
            using pointer = CC*;
            using reference = CC&;
            using value_type = Clr;

            ImageIterator() = default;
            explicit ImageIterator(CI& image, std::int64_t index) noexcept: image_{&image}, index_{index} {}

            operator ImageIterator<Img, Clr, true>() const noexcept requires (! Const) {
                return ImageIterator<Img, Clr, true>{*image_, index_};
            }

            CC& operator*() const noexcept { return image_->pixel_data()[index_]; }
            CC* operator->() const noexcept { return &**this; }
            ImageIterator& operator++() noexcept { ++index_; return *this; }
            ImageIterator operator++(int) noexcept { auto i = *this; ++*this; return i; }
            ImageIterator& operator--() noexcept { --index_; return *this; }
            ImageIterator operator--(int) noexcept { auto i = *this; --*this; return i; }
            friend bool operator==(const ImageIterator& i, const ImageIterator& j) = default;

            ImageIterator& move(int axis, int distance = 1) noexcept {
                std::int64_t d = distance;
                if ((axis & 1) == 1) {
                    d *= image_->width();
                }
                index_ += d;
                return *this;
            }

            Int2 pos() const noexcept {
                auto x = static_cast<int>(index_ % image_->width());
                auto y = static_cast<int>(index_ / image_->width());
                return {x, y};
            }

        };

    }

    template <ColourType CT, ImageFlags Flags>
    class Image {

    public:

        using channel_type = typename CT::value_type;
        using colour_space = typename CT::colour_space;
        using colour_type = CT;
        using iterator = Detail::ImageIterator<Image, colour_type, false>;
        using const_iterator = Detail::ImageIterator<Image, colour_type, true>;

        static constexpr int channels = colour_type::channels;
        static constexpr ColourLayout colour_layout = CT::layout;
        static constexpr bool has_alpha = colour_type::has_alpha;
        static constexpr bool is_bottom_up = has_bit(Flags, ImageFlags::invert);
        static constexpr bool is_top_down = ! is_bottom_up;
        static constexpr bool is_hdr = colour_type::is_hdr;
        static constexpr bool is_linear = colour_type::is_linear;
        static constexpr bool is_premultiplied = has_bit(Flags, ImageFlags::pma);

        static_assert(colour_type::can_premultiply || ! is_premultiplied);

        Image() noexcept: pix_{}, shape_{0, 0} {}
        explicit Image(Int2 shape) { reset(shape); }
        explicit Image(Int2 shape, colour_type c) { reset(shape, c); }
        explicit Image(int w, int h) { reset(w, h); }
        explicit Image(int w, int h, colour_type c) { reset(w, h, c); }

        ~Image() noexcept = default;
        Image(const Image& img) { reset(img.shape()); std::memcpy(channel_data(), img.channel_data(), bytes()); }
        Image(Image&& img) noexcept: pix_{std::move(img.pix_)}, shape_{img.shape_} { img.shape_ = {0, 0}; }
        Image& operator=(const Image& img) { Image copy(img); swap(copy); return *this; }
        Image& operator=(Image&& img) noexcept { Image copy(std::move(img)); swap(copy); return *this; }

        colour_type& operator[](Int2 p) noexcept { return (*this)[p.x(), p.y()]; }
        const colour_type& operator[](Int2 p) const noexcept { return (*this)[p.x(), p.y()]; }
        colour_type& operator[](int x, int y) noexcept { return *locate(x, y); }
        const colour_type& operator[](int x, int y) const noexcept { return *locate(x, y); }

        iterator begin() noexcept { return iterator(*this, 0); }
        const_iterator begin() const noexcept { return const_iterator(*this, 0); }
        iterator end() noexcept { return iterator(*this, std::int64_t(size())); }
        const_iterator end() const noexcept { return const_iterator(*this, std::int64_t(size())); }
        channel_type* channel_data() noexcept { return pixel_data()->begin(); }
        const channel_type* channel_data() const noexcept { return pixel_data()->begin(); }
        CT* pixel_data() noexcept { return pix_.get(); }
        const CT* pixel_data() const noexcept { return pix_.get(); }

        iterator bottom_left() noexcept { return locate(0, is_top_down ? height() - 1 : 0); }
        const_iterator bottom_left() const noexcept { return locate(0, is_top_down ? height() - 1 : 0); }
        iterator bottom_right() noexcept { return locate(width() - 1, is_top_down ? height() - 1 : 0); }
        const_iterator bottom_right() const noexcept { return locate(width() - 1, is_top_down ? height() - 1 : 0); }
        iterator top_left() noexcept { return locate(0, is_top_down ? 0 : height() - 1); }
        const_iterator top_left() const noexcept { return locate(0, is_top_down ? 0 : height() - 1); }
        iterator top_right() noexcept { return locate(width() - 1, is_top_down ? 0 : height() - 1); }
        const_iterator top_right() const noexcept { return locate(width() - 1, is_top_down ? 0 : height() - 1); }

        void clear() noexcept { pix_.reset(); shape_ = {0, 0}; }
        void fill(colour_type c) noexcept { std::ranges::fill(*this, c); }

        void load(const std::filesystem::path& file);
        void save(const std::filesystem::path& file, int quality = 90) const;

        iterator locate(Int2 p) noexcept { return locate(p.x(), p.y()); }
        const_iterator locate(Int2 p) const noexcept { return locate(p.x(), p.y()); }
        iterator locate(int x, int y) noexcept { return iterator(*this, make_index(x, y)); }
        const_iterator locate(int x, int y) const noexcept { return const_iterator(*this, make_index(x, y)); }

        Image<colour_type, Flags | ImageFlags::pma> multiply_alpha() const requires (colour_type::can_premultiply && ! is_premultiplied);
        Image<colour_type, Flags & ~ ImageFlags::pma> unmultiply_alpha() const requires (colour_type::can_premultiply && is_premultiplied);

        void reset(Int2 new_shape);
        void reset(Int2 new_shape, colour_type c) { reset(new_shape); fill(c); }
        void reset(int w, int h) { reset(Int2{w, h}); }
        void reset(int w, int h, colour_type c) { reset(Int2{w, h}, c); }
        void resize(Int2 new_shape, ImageResize rflags = ImageResize::none);
        void resize(double scale, ImageResize rflags = ImageResize::none);
        Image resized(Int2 new_shape, ImageResize rflags = ImageResize::none) const;
        Image resized(double scale, ImageResize rflags = ImageResize::none) const;
        Image segment(Box_i2 box) const;
        Box_i2 extent() const noexcept { return {Int2::null(), shape_}; }
        Int2 shape() const noexcept { return shape_; }
        bool empty() const noexcept { return ! pix_; }
        int width() const noexcept { return shape_.x(); }
        int height() const noexcept { return shape_.y(); }
        std::size_t size() const noexcept { return static_cast<std::size_t>(shape_.x()) * static_cast<std::size_t>(shape_.y()); }
        std::size_t bytes() const noexcept { return size() * sizeof(colour_type); }

        void swap(Image& img) noexcept { std::swap(pix_, img.pix_); std::swap(shape_, img.shape_); }
        friend void swap(Image& a, Image& b) noexcept { a.swap(b); }

        friend bool operator==(const Image& a, const Image& b) noexcept {
            return a.shape_ == b.shape_ && std::memcmp(a.pix_.get(), b.pix_.get(), a.bytes()) == 0;
        }

    private:

        std::unique_ptr<colour_type, Detail::FreeMem> pix_;
        Int2 shape_;

        std::int64_t make_index(int x, int y) const noexcept { return static_cast<std::int64_t>(shape_.x()) * y + x; }

    };

    template <typename C1, ImageFlags F1, typename C2, ImageFlags F2>
    void convert_image(const Image<C1, F1>& in, Image<C2, F2>& out) {

        using Img1 = Image<C1, F1>;
        using Img2 = Image<C2, F2>;

        if constexpr (std::is_same_v<Img1, Img2>) {

            out = in;

        } else if constexpr (Img1::is_premultiplied) {

            auto linear_in = in.unmultiply_alpha();
            convert_image(linear_in, out);

        } else if constexpr (Img2::is_premultiplied) {

            Image<C2, F2 & ~ ImageFlags::pma> linear_out;
            convert_image(in, linear_out);
            out = linear_out.multiply_alpha();

        } else {

            Img2 result{in.shape()};
            int dy, y1, y2;

            if constexpr (Img1::is_top_down == Img2::is_top_down) {
                dy = 1;
                y1 = 0;
                y2 = in.height();
            } else {
                dy = -1;
                y1 = in.height() - 1;
                y2 = -1;
            }

            for (auto y = y1; y != y2; y += dy) {
                auto i = in.locate(0, y);
                auto j = result.locate(0, y);
                for (int x = 0; x < in.width(); ++x, ++i, ++j) {
                    convert_colour(*i, *j);
                }
            }

            out = std::move(result);

        }

    }

    template <ColourType CT, ImageFlags Flags>
    void Image<CT, Flags>::load(const std::filesystem::path& file) {

        Int2 shape;

        if constexpr (std::is_same_v<channel_type, std::uint8_t>) {

            auto image_ptr = Detail::load_image_8(file, shape);
            Image<Rgba8> image(shape);
            std::memcpy(image.channel_data(), image_ptr.get(), image.bytes());
            convert_image(image, *this);

        } else if constexpr (std::is_same_v<channel_type, std::uint16_t>) {

            auto image_ptr = Detail::load_image_16(file, shape);
            Image<Rgba16> image(shape);
            std::memcpy(image.channel_data(), image_ptr.get(), image.bytes());
            convert_image(image, *this);

        } else {

            auto image_ptr = Detail::load_image_hdr(file, shape);
            Image<Rgbaf> image(shape);
            std::memcpy(image.channel_data(), image_ptr.get(), image.bytes());
            convert_image(image, *this);

        }

    }

    template <ColourType CT, ImageFlags Flags>
    void Image<CT, Flags>::save(const std::filesystem::path& file, int quality) const {

        auto format = ascii_lowercase(file.extension().string());

        if (format == ".hdr" || format == ".rgbe") {

            Image<Rgbaf> image;
            convert_image(*this, image);
            Detail::save_image_hdr(image, file);

        } else {

            Image<Rgba8> image;
            convert_image(*this, image);
            Detail::save_image_8(image, file, format, quality);

        }

    }

    template <ColourType CT, ImageFlags Flags>
    Image<CT, Flags | ImageFlags::pma> Image<CT, Flags>::multiply_alpha() const
    requires (CT::can_premultiply && ! is_premultiplied) {

        Image<colour_type, Flags | ImageFlags::pma> result(shape());
        auto out = result.begin();

        for (auto& pixel: *this) {
            *out++ = pixel.multiply_alpha();
        }

        return result;

    }

    template <ColourType CT, ImageFlags Flags>
    Image<CT, Flags & ~ ImageFlags::pma> Image<CT, Flags>::unmultiply_alpha() const
    requires (CT::can_premultiply && is_premultiplied) {

        Image<colour_type, Flags & ~ ImageFlags::pma> result(shape());
        auto out = result.begin();

        for (auto& pixel: *this) {
            *out++ = pixel.unmultiply_alpha();
        }

        return result;

    }

    template <ColourType CT, ImageFlags Flags>
    void Image<CT, Flags>::reset(Int2 new_shape) {

        if (new_shape == Int2(0, 0)) {

            clear();

        } else if (new_shape.x() <= 0 || new_shape.y() <= 0) {

            throw std::invalid_argument{std::format("Invalid image dimensions: {}", new_shape)};

        } else {

            std::size_t n_bytes = std::size_t(new_shape.x()) * std::size_t(new_shape.y()) * sizeof(colour_type);
            auto ptr = std::malloc(n_bytes);

            if (ptr == nullptr) {
                throw std::bad_alloc();
            }

            pix_.reset(static_cast<colour_type*>(ptr));
            shape_ = new_shape;

        }

    }

    template <ColourType CT, ImageFlags Flags>
    void Image<CT, Flags>::resize(Int2 new_shape, ImageResize rflags) {
        auto img = resized(new_shape, rflags);
        *this = std::move(img);
    }

    template <ColourType CT, ImageFlags Flags>
    void Image<CT, Flags>::resize(double scale, ImageResize rflags) {
        auto img = resized(scale, rflags);
        *this = std::move(img);
    }

    template <ColourType CT, ImageFlags Flags>
    Image<CT, Flags> Image<CT, Flags>::resized(Int2 new_shape, ImageResize rflags) const {

        static constexpr int stbir_flag_alpha_premultiplied  = 1;
        static constexpr int stbir_edge_clamp                = 1;
        static constexpr int stbir_edge_wrap                 = 3;
        static constexpr int stbir_filter_default            = 0;
        static constexpr int stbir_colorspace_linear         = 0;
        static constexpr int stbir_colorspace_srgb           = 1;

        using working_channel = std::conditional_t<std::is_same_v<channel_type, std::uint8_t>
            || std::is_same_v<channel_type, std::uint16_t>, channel_type, float>;
        using working_space = std::conditional_t<std::is_same_v<colour_space, sRGB>, sRGB, LinearRGB>;
        using working_colour = Colour<working_channel, working_space, colour_layout>;
        using working_image = Image<working_colour, Flags>;

        auto fail = [new_shape] {
            throw std::invalid_argument{std::format("Invalid image dimensions: {}", new_shape)};
        };

        if (new_shape.x() < 0 || new_shape.y() < 0) {
            fail();
        }

        if (new_shape.x() == 0 && new_shape.y() == 0) {
            fail();
        }

        if ((new_shape.x() == 0 || new_shape.y() == 0) && has_bit(rflags, ImageResize::unlock)) {
            fail();
        }

        Int2 actual_shape = new_shape;

        if (! has_bit(rflags, ImageResize::unlock)) {

            Double2 rshape{new_shape.x(), new_shape.y()};
            auto ratio = static_cast<double>(width()) / static_cast<double>(height());

            if (new_shape.x() == 0) {
                rshape.x() = rshape.y() * ratio;
            } else if (new_shape.y() == 0) {
                rshape.y() = rshape.x() / ratio;
            } else {
                rshape.x() = std::min(rshape.x(), rshape.y() * ratio);
                rshape.y() = std::min(rshape.y(), rshape.x() / ratio);
            }

            actual_shape.x() = static_cast<int>(std::lround(rshape.x()));
            actual_shape.y() = static_cast<int>(std::lround(rshape.y()));

        }

        int stb_flags = is_premultiplied ? stbir_flag_alpha_premultiplied : 0;
        int stb_edge = has_bit(rflags, ImageResize::wrap) ? stbir_edge_wrap : stbir_edge_clamp;
        int stb_filter = stbir_filter_default;
        int stb_space = std::is_same_v<colour_space, sRGB> ? stbir_colorspace_srgb : stbir_colorspace_linear;

        working_image working_input;
        convert_image(*this, working_input);
        working_image working_output(actual_shape);

        if constexpr (std::is_same_v<channel_type, std::uint8_t>) {
            Detail::resize_image_8(working_input.channel_data(), shape_, working_output.channel_data(), actual_shape,
                working_colour::channels, working_colour::alpha_index, stb_flags, stb_edge, stb_filter, stb_space);
        } else if constexpr (std::is_same_v<channel_type, std::uint16_t>) {
            Detail::resize_image_16(working_input.channel_data(), shape_, working_output.channel_data(), actual_shape,
                working_colour::channels, working_colour::alpha_index, stb_flags, stb_edge, stb_filter, stb_space);
        } else {
            Detail::resize_image_hdr(working_input.channel_data(), shape_, working_output.channel_data(), actual_shape,
                working_colour::channels, working_colour::alpha_index, stb_flags, stb_edge, stb_filter, stb_space);
        }

        Image result;
        convert_image(working_output, result);

        return result;

    }

    template <ColourType CT, ImageFlags Flags>
    Image<CT, Flags> Image<CT, Flags>::resized(double scale, ImageResize rflags) const {

        if (scale <= 0) {
            throw std::invalid_argument{std::format("Invalid image scale factor: {}", scale)};
        }

        auto w = static_cast<int>(std::lround(scale * width()));
        auto h = static_cast<int>(std::lround(scale * height()));

        return resized(Int2{w, h}, rflags | ImageResize::unlock);

    }

    template <ColourType CT, ImageFlags Flags>
    Image<CT, Flags> Image<CT, Flags>::segment(Box_i2 box) const {

        if (! extent().contains(box)) {
            throw std::invalid_argument("Image segment box is not contained within image");
        }

        Image img{box.shape()};

        for (auto y2 = 0; y2 < box.shape().y(); ++y2) {
            int y1 = y2 + box.base().y();
            auto i1 = locate(box.base().x(), y1);
            auto i2 = locate(box.apex().x(), y1);
            auto j = img.locate(0, y2);
            std::copy(i1, i2, j);
        }

        return img;

    }

}
