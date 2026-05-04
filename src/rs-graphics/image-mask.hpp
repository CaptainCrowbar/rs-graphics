#pragma once

// This header is not part of the public interface

#include "rs-graphics/colour.hpp"
#include "rs-graphics/image.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

namespace RS::Graphics::Detail {

    template <Arithmetic T>
    class ImageMask {

    public:

        using value_type = T;

        static constexpr T scale = Integral<T> ? std::numeric_limits<T>::max() : T{1};

        ImageMask() = default;
        explicit ImageMask(Int2 shape) noexcept;
        template <typename Del> ImageMask(Int2 shape, T* ptr, Del del) noexcept;

        T& operator[](Int2 p) noexcept { return ptr_.get()[point_to_index(p)]; }
        const T& operator[](Int2 p) const noexcept { return ptr_.get()[point_to_index(p)]; }
        T* begin() noexcept { return ptr_.get(); }
        const T* begin() const noexcept { return ptr_.get(); }
        T* end() noexcept { return begin() + area(); }
        const T* end() const noexcept { return begin() + area(); }
        std::size_t area() const noexcept;
        bool empty() const noexcept { return ! ptr_ || shape_.x() <= 0 || shape_.y() <= 0; }
        Int2 shape() const noexcept { return shape_; }

        template <ColourType C, ImageFlags F>
            requires C::is_linear
            void make_image(Image<C, F>& image, C foreground, C background) const;
        template <ColourType C, ImageFlags F>
            requires C::is_linear
            void onto_image(Image<C, F>& image, Int2 offset, C colour) const;

    private:

        Int2 shape_;
        std::shared_ptr<T[]> ptr_;

        std::size_t point_to_index(Int2 p) const noexcept;

        template <ColourType C> static C blend(C fg, C bg, T alpha, Pma pma) noexcept;

    };

        using ByteMask = ImageMask<unsigned char>;
        using HdrMask = ImageMask<float>;

        template <Arithmetic T>
        ImageMask<T>::ImageMask(Int2 shape) noexcept:
        shape_{shape},
        ptr_{new T[area()]} {
            std::memset(ptr_.get(), 0, area() * sizeof(T));
        }

        template <Arithmetic T>
        template <typename Del>
        ImageMask<T>::ImageMask(Int2 shape, T* ptr, Del del) noexcept:
        shape_{shape},
        ptr_{ptr, del} {}

        template <Arithmetic T>
        std::size_t ImageMask<T>::area() const noexcept {
            return static_cast<std::size_t>(shape_.x()) * static_cast<std::size_t>(shape_.y());
        }

        template <Arithmetic T>
        template <ColourType C, ImageFlags F>
        requires C::is_linear
        void ImageMask<T>::make_image(Image<C, F>& image, C foreground, C background) const {

            static constexpr auto pma = Image<C, F>::is_premultiplied ? Pma::result : Pma::none;

            image.reset(shape());
            auto out = image.begin();

            for (T alpha: *this) {
                *out++ = blend(foreground, background, alpha, pma);
            }

        }

        template <Arithmetic T>
        template <ColourType C, ImageFlags F>
        requires C::is_linear
        void ImageMask<T>::onto_image(Image<C, F>& image, Int2 offset, C colour) const {

            static constexpr auto pma = Image<C, F>::is_premultiplied ? Pma::second | Pma::result : Pma::none;

            auto mask_x1 = std::max(0, - offset.x());
            auto mask_y1 = std::max(0, - offset.y());
            auto mask_x2 = std::min(shape().x(), image.width() - offset.x());
            auto mask_y2 = std::min(shape().y(), image.height() - offset.y());

            if (mask_x1 >= mask_x2 || mask_y1 >= mask_y2) {
                return;
            }

            auto image_x1 = mask_x1 + offset.x();
            auto image_y1 = mask_y1 + offset.y();
            Int2 p; // Point on mask
            Int2 q; // Point on image

            for (p.y() = mask_y1, q.y() = image_y1; p.y() < mask_y2; ++p.y(), ++q.y()) {
                for (p.x() = mask_x1, q.x() = image_x1; p.x() < mask_x2; ++p.x(), ++q.x()) {
                    image[q] = blend(colour, image[q], (*this)[p], pma);
                }
            }

        }

        template <Arithmetic T>
        std::size_t ImageMask<T>::point_to_index(Int2 p) const noexcept {
            return static_cast<std::size_t>(shape_.x()) * static_cast<std::size_t>(p.y())
                + static_cast<std::size_t>(p.x());
        }

        template <Arithmetic T>
        template <ColourType C>
        C ImageMask<T>::blend(C fg, C bg, T alpha, Pma pma) noexcept {

            using V = typename C::value_type;
            using A = std::conditional_t<(sizeof(V) < sizeof(double)), float, double>;

            auto a = static_cast<A>(alpha) / static_cast<A>(scale);

            if constexpr (C::has_alpha) {

                auto fga1 = a * static_cast<A>(fg.alpha());
                V fga2;

                if constexpr (Integral<V>) {
                    fga2 = static_cast<V>(static_cast<A>(C::scale) * fga1 + static_cast<A>(0.5));
                } else {
                    fga2 = static_cast<V>(fga1);
                }

                auto modified_fg = fg;
                modified_fg.alpha() = fga2;
                auto result = alpha_blend(modified_fg, bg);

                if (pma != Pma::none) {
                    result.multiply_alpha();
                }

                return result;

            } else {

                return lerp(bg, fg, a);

            }

        }

}
