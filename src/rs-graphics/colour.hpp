#pragma once

#include "rs-graphics/colour-space.hpp"
#include "rs-core/arithmetic.hpp"
#include "rs-core/enum.hpp"
#include "rs-core/linear-algebra.hpp"
#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace RS::Graphics {

    RS_ENUM(ColourLayout, std::uint8_t,
        forward,
        forward_alpha,
        alpha_forward,
        reverse,
        reverse_alpha,
        alpha_reverse
    )

    RS_BITMASK(Pma, std::uint8_t,
        none    = 0,
        first   = 1,
        second  = 2,
        result  = 4,
        all     = 7,
    )

    template <ColourChannel VT, ColourSpace CS, ColourLayout CL> class Colour;

    namespace Detail {

        template <typename CT> struct IsColourType: std::false_type {};
        template <ColourChannel VT, ColourSpace CS, ColourLayout CL> struct IsColourType<Colour<VT, CS, CL>>: std::true_type {};

        template <ColourSpace CS, char CH, int Offset = 0>
        struct ColourSpaceChannelIndex {
            constexpr static int get() noexcept {
                if constexpr (Offset >= CS::count) {
                    return -1;
                } else if constexpr (CS::channels[Offset] == CH) {
                    return Offset;
                } else {
                    return ColourSpaceChannelIndex<CS, CH, Offset + 1>::get();
                }
            }
        };

        template <ColourSpace CS, char CH, ColourLayout CL>
        struct ColourChannelIndex {
            constexpr static int get() noexcept {
                constexpr int cs_index = ColourSpaceChannelIndex<CS, CH>::get();
                if constexpr (cs_index == -1) {
                    return -1;
                } else if constexpr (CL == ColourLayout::forward || CL == ColourLayout::forward_alpha) {
                    return cs_index;
                } else if constexpr (CL == ColourLayout::alpha_forward) {
                    return cs_index + 1;
                } else if constexpr (CL == ColourLayout::reverse || CL == ColourLayout::reverse_alpha) {
                    return CS::count - cs_index - 1;
                } else {
                    return CS::count - cs_index;
                }
            }
        };

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL, bool IsLinear = LinearColourSpace<CS>>
        class ColourArithmetic {

        private:

            constexpr static bool ca_has_alpha = CL != ColourLayout::forward && CL != ColourLayout::reverse;
            constexpr static int ca_channels = CS::count + int(ca_has_alpha);

            using ca_colour = Colour<VT, CS, CL>;
            using ca_vector = Vector<VT, ca_channels>;

        public:

            constexpr friend ca_colour operator+(ca_colour c) noexcept { return c; }
            constexpr friend ca_colour operator-(ca_colour c) noexcept { return ca_colour(- c.as_vector()); }
            constexpr friend ca_colour operator+(ca_colour a, ca_colour b) noexcept { return ca_colour(a.as_vector() + b.as_vector()); }
            constexpr friend ca_colour operator-(ca_colour a, ca_colour b) noexcept { return ca_colour(a.as_vector() - b.as_vector()); }
            constexpr friend ca_colour operator*(ca_colour a, VT b) noexcept { return ca_colour(a.as_vector() * b); }
            constexpr friend ca_colour operator*(VT a, ca_colour b) noexcept { return ca_colour(a * b.as_vector()); }
            constexpr friend ca_colour operator/(ca_colour a, VT b) noexcept { return ca_colour(a.as_vector() / b); }
            constexpr friend ca_colour operator*(ca_colour a, ca_vector b) noexcept { return ca_colour(a.as_vector() * b); }
            constexpr friend ca_colour operator*(ca_vector a, ca_colour b) noexcept { return ca_colour(a * b.as_vector()); }
            constexpr friend ca_colour operator/(ca_colour a, ca_vector b) noexcept { return ca_colour(a.as_vector() / b); }
            constexpr friend ca_colour& operator+=(ca_colour& a, ca_colour b) noexcept { return a = a + b; }
            constexpr friend ca_colour& operator-=(ca_colour& a, ca_colour b) noexcept { return a = a - b; }
            constexpr friend ca_colour& operator*=(ca_colour& a, VT b) noexcept { return a = a * b; }
            constexpr friend ca_colour& operator/=(ca_colour& a, VT b) noexcept { return a = a / b; }
            constexpr friend ca_colour& operator*=(ca_colour& a, ca_vector b) noexcept { return a = a * b; }
            constexpr friend ca_colour& operator/=(ca_colour& a, ca_vector b) noexcept { return a = a / b; }

        };

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        class ColourArithmetic<VT, CS, CL, false> {};

        template <ColourChannel T>
        using FloatingChannelType =
            std::conditional_t<std::is_floating_point_v<T>, T,
            std::conditional_t<(sizeof(T) < sizeof(float)), float,
            std::conditional_t<(sizeof(T) < sizeof(double)), double, long double>>>;

        template <ColourChannel T1, ColourChannel T2>
        using WorkingChannelType =
            std::conditional_t<(sizeof(FloatingChannelType<T1>) >= sizeof(FloatingChannelType<T2>)),
            FloatingChannelType<T1>, FloatingChannelType<T2>>;

        template <ColourChannel WT, ColourChannel T1>
        constexpr WT channel_to_working_type(T1 c, T1 scale) noexcept {
            return static_cast<WT>(c) / static_cast<WT>(scale);
        }

        template <ColourChannel T2, ColourChannel WT>
        constexpr T2 working_type_to_channel(WT w, T2 scale) noexcept {
            w *= static_cast<WT>(scale);
            if constexpr (std::is_integral_v<T2>) {
                return static_cast<T2>(std::round(w));
            } else {
                return static_cast<T2>(w);
            }
        }

        template <ColourChannel T2, ColourChannel T1>
        constexpr T2 round_channel(T1 t) noexcept {
            if constexpr (std::is_integral_v<T2>) {
                return static_cast<T2>(std::round(t));
            } else {
                return static_cast<T2>(t);
            }
        }

    }

    template <typename CT> concept ColourType = Detail::IsColourType<CT>::value;
    template <typename CT> concept LinearColourType = ColourType<CT> && CT::is_linear;

    // Don't use single letter template parameters here because they may
    // collide with channel access function names.

    template <ColourChannel VT = float, ColourSpace CS = LinearRGB, ColourLayout CL = ColourLayout::forward_alpha>
    class Colour:
    public Detail::ColourArithmetic<VT, CS, CL> {

    public:

        constexpr static int colour_space_channels = CS::count;
        constexpr static int alpha_index = CL == ColourLayout::alpha_forward || CL == ColourLayout::alpha_reverse ? 0 :
            CL == ColourLayout::forward_alpha || CL == ColourLayout::reverse_alpha ? colour_space_channels : -1;
        constexpr static bool has_alpha = alpha_index != -1;
        constexpr static bool can_premultiply = has_alpha && LinearColourSpace<CS> && RgbColourSpace<CS>
            && UnitColourSpace<CS> && ! PolarColourSpace<CS>;
        constexpr static int channels = colour_space_channels + int(has_alpha);
        constexpr static bool is_hdr = std::is_floating_point_v<VT>;
        constexpr static bool is_linear = LinearColourSpace<CS>;
        constexpr static ColourLayout layout = CL;
        constexpr static VT scale = is_hdr || (! PolarColourSpace<CS> && ! UnitColourSpace<CS>) ?
            VT{1} : std::numeric_limits<VT>::max();

    private:

        template <char Lit> constexpr static bool has_channel = Detail::ColourChannelIndex<CS, Lit, CL>::get() != -1;
        template <typename... Args> constexpr static bool has_alpha_args = sizeof...(Args) + 2 == channels;
        template <typename... Args> constexpr static bool has_nonalpha_args = has_alpha && sizeof...(Args) + 2 == colour_space_channels;
        constexpr static bool is_rgb = RgbColourSpace<CS>;
        constexpr static bool is_rgba = is_rgb && has_alpha;

    public:

        using colour_space = CS;
        using const_iterator = const VT*;
        using iterator = VT*;
        using partial_vector_type = Vector<VT, colour_space_channels>;
        using value_type = VT;
        using vector_type = Vector<VT, channels>;

        constexpr Colour() noexcept = default;
        constexpr explicit Colour(VT x) noexcept: vec_(x) { if constexpr (has_alpha) { alpha() = scale; } }
        constexpr explicit Colour(VT x, VT a) noexcept requires has_alpha: vec_{x} { alpha() = a; }
        constexpr explicit Colour(vector_type v) noexcept: vec_(v) {}
        template <std::convertible_to<VT>... Args> constexpr explicit Colour(VT x, VT y, Args... args) noexcept requires has_alpha_args<Args...>;
        template <std::convertible_to<VT>... Args> constexpr explicit Colour(VT x, VT y, Args... args) noexcept requires has_nonalpha_args<Args...>;
        explicit Colour(std::string_view str) requires RgbColourSpace<CS>;

        #define RS_GRAPHICS_COLOUR_CHANNEL_(Ch, Lit) \
            constexpr VT& Ch() noexcept requires has_channel<Lit> { \
                return vec_[Detail::ColourChannelIndex<CS, Lit, CL>::get()]; \
            } \
            constexpr const VT& Ch() const noexcept requires has_channel<Lit> { \
                return vec_[Detail::ColourChannelIndex<CS, Lit, CL>::get()]; \
            }

        RS_GRAPHICS_COLOUR_CHANNEL_(a, 'a')  RS_GRAPHICS_COLOUR_CHANNEL_(A, 'A')
        RS_GRAPHICS_COLOUR_CHANNEL_(b, 'b')  RS_GRAPHICS_COLOUR_CHANNEL_(B, 'B')
        RS_GRAPHICS_COLOUR_CHANNEL_(c, 'c')  RS_GRAPHICS_COLOUR_CHANNEL_(C, 'C')
        RS_GRAPHICS_COLOUR_CHANNEL_(d, 'd')  RS_GRAPHICS_COLOUR_CHANNEL_(D, 'D')
        RS_GRAPHICS_COLOUR_CHANNEL_(e, 'e')  RS_GRAPHICS_COLOUR_CHANNEL_(E, 'E')
        RS_GRAPHICS_COLOUR_CHANNEL_(f, 'f')  RS_GRAPHICS_COLOUR_CHANNEL_(F, 'F')
        RS_GRAPHICS_COLOUR_CHANNEL_(g, 'g')  RS_GRAPHICS_COLOUR_CHANNEL_(G, 'G')
        RS_GRAPHICS_COLOUR_CHANNEL_(h, 'h')  RS_GRAPHICS_COLOUR_CHANNEL_(H, 'H')
        RS_GRAPHICS_COLOUR_CHANNEL_(i, 'i')  RS_GRAPHICS_COLOUR_CHANNEL_(I, 'I')
        RS_GRAPHICS_COLOUR_CHANNEL_(j, 'j')  RS_GRAPHICS_COLOUR_CHANNEL_(J, 'J')
        RS_GRAPHICS_COLOUR_CHANNEL_(k, 'k')  RS_GRAPHICS_COLOUR_CHANNEL_(K, 'K')
        RS_GRAPHICS_COLOUR_CHANNEL_(l, 'l')  RS_GRAPHICS_COLOUR_CHANNEL_(L, 'L')
        RS_GRAPHICS_COLOUR_CHANNEL_(m, 'm')  RS_GRAPHICS_COLOUR_CHANNEL_(M, 'M')
        RS_GRAPHICS_COLOUR_CHANNEL_(n, 'n')  RS_GRAPHICS_COLOUR_CHANNEL_(N, 'N')
        RS_GRAPHICS_COLOUR_CHANNEL_(o, 'o')  RS_GRAPHICS_COLOUR_CHANNEL_(O, 'O')
        RS_GRAPHICS_COLOUR_CHANNEL_(p, 'p')  RS_GRAPHICS_COLOUR_CHANNEL_(P, 'P')
        RS_GRAPHICS_COLOUR_CHANNEL_(q, 'q')  RS_GRAPHICS_COLOUR_CHANNEL_(Q, 'Q')
        RS_GRAPHICS_COLOUR_CHANNEL_(r, 'r')  RS_GRAPHICS_COLOUR_CHANNEL_(R, 'R')
        RS_GRAPHICS_COLOUR_CHANNEL_(s, 's')  RS_GRAPHICS_COLOUR_CHANNEL_(S, 'S')
        RS_GRAPHICS_COLOUR_CHANNEL_(t, 't')  RS_GRAPHICS_COLOUR_CHANNEL_(T, 'T')
        RS_GRAPHICS_COLOUR_CHANNEL_(u, 'u')  RS_GRAPHICS_COLOUR_CHANNEL_(U, 'U')
        RS_GRAPHICS_COLOUR_CHANNEL_(v, 'v')  RS_GRAPHICS_COLOUR_CHANNEL_(V, 'V')
        RS_GRAPHICS_COLOUR_CHANNEL_(w, 'w')  RS_GRAPHICS_COLOUR_CHANNEL_(W, 'W')
        RS_GRAPHICS_COLOUR_CHANNEL_(x, 'x')  RS_GRAPHICS_COLOUR_CHANNEL_(X, 'X')
        RS_GRAPHICS_COLOUR_CHANNEL_(y, 'y')  RS_GRAPHICS_COLOUR_CHANNEL_(Y, 'Y')
        RS_GRAPHICS_COLOUR_CHANNEL_(z, 'z')  RS_GRAPHICS_COLOUR_CHANNEL_(Z, 'Z')

        #undef RS_GRAPHICS_COLOUR_CHANNEL_

        VT& operator[](std::size_t i) noexcept { return vec_[i]; }
        const VT& operator[](std::size_t i) const noexcept { return vec_[i]; }
        constexpr VT& alpha() noexcept requires has_alpha;
        constexpr const VT& alpha() const noexcept;
        constexpr VT& cs(std::size_t i) noexcept { return vec_[space_to_layout_index(i)]; }
        constexpr const VT& cs(std::size_t i) const noexcept { return vec_[space_to_layout_index(i)]; }
        constexpr vector_type as_vector() const noexcept { return vec_; }
        constexpr partial_vector_type partial_vector() const noexcept { return partial_vector_type(vec_.begin()); }
        constexpr VT* begin() noexcept { return vec_.begin(); }
        constexpr const VT* begin() const noexcept { return vec_.begin(); }
        constexpr VT* end() noexcept { return vec_.end(); }
        constexpr const VT* end() const noexcept { return vec_.end(); }
        constexpr void clamp() noexcept;
        constexpr Colour clamped() const noexcept;
        constexpr bool is_clamped() const noexcept;
        constexpr bool empty() const noexcept { return false; }
        std::size_t hash() const noexcept { return vec_.hash(); }
        std::string hex() const requires RgbColourSpace<CS>;
        constexpr Colour multiply_alpha() const noexcept requires can_premultiply;
        constexpr Colour unmultiply_alpha() const noexcept requires can_premultiply;
        constexpr std::size_t size() const noexcept { return std::size_t(channels); }

        static Colour clear() noexcept requires has_alpha { return Colour{}; }
        static Colour black() noexcept requires is_rgb { return Colour{0}; }
        static Colour white() noexcept requires is_rgb { return Colour{scale}; }
        static Colour red() noexcept requires is_rgb { return Colour{scale, 0, 0}; }
        static Colour yellow() noexcept requires is_rgb { return Colour{scale, scale, 0}; }
        static Colour green() noexcept requires is_rgb { return Colour{0, scale, 0}; }
        static Colour cyan() noexcept requires is_rgb { return Colour{0, scale, scale}; }
        static Colour blue() noexcept requires is_rgb { return Colour{0, 0, scale}; }
        static Colour magenta() noexcept requires is_rgb { return Colour{scale, 0, scale}; }

        constexpr friend bool operator==(Colour a, Colour b) noexcept { return a.vec_ == b.vec_; }

    private:

        friend class Detail::ColourArithmetic<VT, CS, CL>;

        vector_type vec_;

        constexpr static std::size_t space_to_layout_index(std::size_t cs) noexcept;

    };

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        template <std::convertible_to<VT>... Args>
        constexpr Colour<VT, CS, CL>::Colour(VT x, VT y, Args... args) noexcept
        requires has_alpha_args<Args...>:
        vec_(x, y, static_cast<VT>(args)...) {}

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        template <std::convertible_to<VT>... Args>
        constexpr Colour<VT, CS, CL>::Colour(VT x, VT y, Args... args) noexcept
        requires has_nonalpha_args<Args...>:
        vec_(x, y, static_cast<VT>(args)..., scale) {}

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        Colour<VT, CS, CL>::Colour(std::string_view str)
        requires RgbColourSpace<CS> {

            Byte4 bytes{0,0,0,255};

            std::size_t i, j, k;
            for (i = 0; i < str.size() && (ascii_ispunct(str[i]) || ascii_isspace(str[i])); ++i) {}
            for (j = i; j < str.size() && ascii_isxdigit(str[j]); ++j) {}
            for (k = j; k < str.size() && (ascii_ispunct(str[k]) || ascii_isspace(str[k])); ++k) {}
            std::size_t digits = j - i;

            if (k != str.size() || (digits != 6 && digits != 8)) {
                throw std::invalid_argument(std::format("Invalid colour: {:?}", str));
            }

            bytes[0] = try_parse_number<std::uint8_t>(str.substr(i, 2), 16);
            bytes[1] = try_parse_number<std::uint8_t>(str.substr(i + 2, 2), 16);
            bytes[2] = try_parse_number<std::uint8_t>(str.substr(i + 4, 2), 16);

            if (digits == 8) {
                bytes[3] = try_parse_number<std::uint8_t>(str.substr(i + 6, 2), 16);
            }

            Vector<VT, 4> vts;

            if constexpr (std::is_same_v<VT, std::uint8_t>) {

                vts = bytes;

            } else if constexpr (std::is_integral_v<VT> && std::is_signed_v<VT>) {

                constexpr static double s = double(scale) / 255;

                for (auto x = 0uz; x < channels; ++x) {
                    vts[x] = static_cast<VT>(std::round(s * double(bytes[x])));
                }

            } else {

                constexpr static VT s = scale / 255;

                for (auto x = 0uz; x < channels; ++x) {
                    vts[x] = s * VT(bytes[x]);
                }

            }

            R() = vts[0];
            G() = vts[1];
            B() = vts[2];

            if constexpr (has_alpha) {
                alpha() = vts[3];
            }

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr VT& Colour<VT, CS, CL>::alpha() noexcept
        requires has_alpha {
            return vec_[alpha_index];
        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr const VT& Colour<VT, CS, CL>::alpha() const noexcept {
            if constexpr (has_alpha) {
                return vec_[alpha_index];
            } else {
                return scale;
            }
        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr void Colour<VT, CS, CL>::clamp() noexcept {

            if constexpr (has_alpha) {

                auto pv = partial_vector();
                clamp_colour<CS>(pv, scale);
                auto offset = static_cast<int>(CL == ColourLayout::alpha_forward || CL == ColourLayout::alpha_reverse);

                for (auto i = 0uz; i < colour_space_channels; ++i) {
                    vec_[i + offset] = pv[i];
                } if (alpha() < 0) {
                    alpha() = 0;
                } else if (alpha() > scale) {
                    alpha() = scale;
                }

            } else {

                clamp_colour<CS>(vec_, scale);

            }

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr Colour<VT, CS, CL> Colour<VT, CS, CL>::clamped() const noexcept {
            auto c = *this;
            c.clamp();
            return c;
        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr bool Colour<VT, CS, CL>::is_clamped() const noexcept {

            if (! is_colour_in_gamut<CS>(partial_vector(), scale)) {
                return false;
            }

            if constexpr (has_alpha) {
                if (alpha() < 0 || alpha() > scale) {
                    return false;
                }
            }

            return true;

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        std::string Colour<VT, CS, CL>::hex() const
        requires RgbColourSpace<CS> {

            Vector<VT, 4> vts{R(), G(), B(), alpha()};
            Byte4 bytes;

            if constexpr (std::is_same_v<VT, std::uint8_t>) {
                bytes = vts;
            } else {
                constexpr static double k = 255 / double(scale);
                for (auto i = 0uz; i < channels; ++i) {
                    bytes[i] = static_cast<std::uint8_t>(std::round(k * double(vts[i])));
                }
            }

            if (has_alpha) {
                return std::format("{:02x}{:02x}{:02x}{:02x}", bytes[0], bytes[1], bytes[2], bytes[3]);
            } else {
                return std::format("{:02x}{:02x}{:02x}", bytes[0], bytes[1], bytes[2]);
            }

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr Colour<VT, CS, CL> Colour<VT, CS, CL>::multiply_alpha() const noexcept
        requires can_premultiply {

            using FT = Detail::FloatingChannelType<VT>;

            auto result = *this;
            auto factor = static_cast<FT>(alpha()) / static_cast<FT>(scale);

            for (auto& x: result.vec_) {
                x = Detail::round_channel<VT>(factor * x);
            }

            result.alpha() = alpha();

            return result;

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr Colour<VT, CS, CL> Colour<VT, CS, CL>::unmultiply_alpha() const noexcept
        requires can_premultiply {

            using FT = Detail::FloatingChannelType<VT>;

            auto result = *this;
            auto factor = static_cast<FT>(scale) / static_cast<FT>(alpha());

            for (auto& x: result.vec_) {
                x = Detail::round_channel<VT>(factor * x);
            }

            result.alpha() = alpha();

            return result;

        }

        template <ColourChannel VT, ColourSpace CS, ColourLayout CL>
        constexpr std::size_t Colour<VT, CS, CL>::space_to_layout_index(std::size_t cs) noexcept {
            if constexpr (CL <= ColourLayout::forward_alpha) {
                return cs;
            } else if constexpr (CL == ColourLayout::alpha_forward) {
                return (cs + 1) % channels;
            } else if constexpr (CL == ColourLayout::reverse_alpha) {
                return (2 * colour_space_channels - cs) % channels;
            } else {
                return channels - cs - 1;
            }
        }

    template <ColourType CT1, ColourType CT2>
    void convert_colour(CT1 in, CT2& out) noexcept {

        using VT1 = typename CT1::value_type;
        using VT2 = typename CT2::value_type;
        using CS1 = typename CT1::colour_space;
        using CS2 = typename CT2::colour_space;

        constexpr static auto CL1 = CT1::layout;
        constexpr static auto CL2 = CT2::layout;

        if constexpr (std::is_same_v<VT1, VT2> && std::is_same_v<CS1, CS2> && CL1 == CL2) {

            out = in;

        } else if constexpr (std::is_same_v<VT1, VT2> && std::is_same_v<CS1, CS2>) {

            for (auto i = 0uz; i < CS1::count; ++i) {
                out.cs(i) = in.cs(i);
            }

            if constexpr (CT2::has_alpha) {
                out.alpha() = in.alpha();
            }

        } else {

            using WT = Detail::WorkingChannelType<VT1, VT2>;
            using WC1 = Colour<WT, CS1, ColourLayout::forward_alpha>;
            using WC2 = Colour<WT, CS2, ColourLayout::forward_alpha>;

            // WT is always floating point, so the scale of WC[12] is always 1

            WC1 wc1;

            for (auto i = 0uz; i < CS1::count; ++i) {
                wc1.cs(i) = Detail::channel_to_working_type<WT>(in.cs(i), CT1::scale);
            }

            wc1.alpha() = Detail::channel_to_working_type<WT>(in.alpha(), CT1::scale);
            auto pvec2 = convert_colour_space<CS1, CS2>(wc1.partial_vector());
            WC2 wc2;

            for (auto i = 0uz; i < CS2::count; ++i) {
                wc2[i] = pvec2[i];
            }

            wc2.alpha() = wc1.alpha();

            for (auto i = 0uz; i < CT2::channels; ++i) {
                out.cs(i) = Detail::working_type_to_channel(wc2.cs(i), CT2::scale);
            }

        }

    }

    template <ColourType CT>
    constexpr CT alpha_blend(CT a, CT b, Pma flags = {}) noexcept
    requires CT::can_premultiply {

        constexpr auto CL = CT::layout;

        using VT = typename CT::value_type;
        using CS = typename CT::colour_space;
        using FT = Detail::FloatingChannelType<VT>;
        using FC = Colour<FT, CS, CL>;

        FC fa, fb, fc;
        convert_colour(a, fa);
        convert_colour(b, fb);

        if ((flags & Pma::first) == Pma::none) {
            fa = fa.multiply_alpha();
        }

        if ((flags & Pma::second) == Pma::none) {
            fb = fb.multiply_alpha();
        }

        for (auto i = 0uz; i < CT::channels; ++i) {
            fc[i] = fa[i] + fb[i] * (1 - fa.alpha());
        }

        if ((flags & Pma::result) == Pma::none) {
            fc = fc.unmultiply_alpha();
        }

        CT c;
        convert_colour(fc, c);

        return c;

    }

    template <ColourType CT, std::floating_point FP>
    constexpr CT lerp(const CT& c1, const CT& c2, FP x) noexcept {
        auto v1 = c1.as_vector();
        auto v2 = c2.as_vector();
        auto v3 = lerp(v1, v2, x);
        return CT{v3};
    }

    using Rgb8 = Colour<std::uint8_t, LinearRGB, ColourLayout::forward>;
    using Rgb16 = Colour<uint16_t, LinearRGB, ColourLayout::forward>;
    using Rgbf = Colour<float, LinearRGB, ColourLayout::forward>;
    using Rgbd = Colour<double, LinearRGB, ColourLayout::forward>;
    using sRgb8 = Colour<std::uint8_t, sRGB, ColourLayout::forward>;
    using sRgb16 = Colour<uint16_t, sRGB, ColourLayout::forward>;
    using sRgbf = Colour<float, sRGB, ColourLayout::forward>;
    using sRgbd = Colour<double, sRGB, ColourLayout::forward>;
    using Rgba8 = Colour<std::uint8_t, LinearRGB, ColourLayout::forward_alpha>;
    using Rgba16 = Colour<uint16_t, LinearRGB, ColourLayout::forward_alpha>;
    using Rgbaf = Colour<float, LinearRGB, ColourLayout::forward_alpha>;
    using Rgbad = Colour<double, LinearRGB, ColourLayout::forward_alpha>;
    using sRgba8 = Colour<std::uint8_t, sRGB, ColourLayout::forward_alpha>;
    using sRgba16 = Colour<uint16_t, sRGB, ColourLayout::forward_alpha>;
    using sRgbaf = Colour<float, sRGB, ColourLayout::forward_alpha>;
    using sRgbad = Colour<double, sRGB, ColourLayout::forward_alpha>;

    static_assert(std::is_standard_layout_v<Rgb8>);
    static_assert(std::is_standard_layout_v<Rgb16>);
    static_assert(std::is_standard_layout_v<Rgbf>);
    static_assert(std::is_standard_layout_v<Rgbd>);
    static_assert(std::is_standard_layout_v<sRgb8>);
    static_assert(std::is_standard_layout_v<sRgb16>);
    static_assert(std::is_standard_layout_v<sRgbf>);
    static_assert(std::is_standard_layout_v<sRgbd>);
    static_assert(std::is_standard_layout_v<Rgba8>);
    static_assert(std::is_standard_layout_v<Rgba16>);
    static_assert(std::is_standard_layout_v<Rgbaf>);
    static_assert(std::is_standard_layout_v<Rgbad>);
    static_assert(std::is_standard_layout_v<sRgba8>);
    static_assert(std::is_standard_layout_v<sRgba16>);
    static_assert(std::is_standard_layout_v<sRgbaf>);
    static_assert(std::is_standard_layout_v<sRgbad>);

    static_assert(sizeof(Rgb8) == 3);
    static_assert(sizeof(Rgb16) == 6);
    static_assert(sizeof(Rgbf) == 12);
    static_assert(sizeof(Rgbd) == 24);
    static_assert(sizeof(sRgb8) == 3);
    static_assert(sizeof(sRgb16) == 6);
    static_assert(sizeof(sRgbf) == 12);
    static_assert(sizeof(sRgbd) == 24);
    static_assert(sizeof(Rgba8) == 4);
    static_assert(sizeof(Rgba16) == 8);
    static_assert(sizeof(Rgbaf) == 16);
    static_assert(sizeof(Rgbad) == 32);
    static_assert(sizeof(sRgba8) == 4);
    static_assert(sizeof(sRgba16) == 8);
    static_assert(sizeof(sRgbaf) == 16);
    static_assert(sizeof(sRgbad) == 32);

}

namespace std {

    template <RS::Graphics::ColourChannel VT, RS::Graphics::ColourSpace CS, RS::Graphics::ColourLayout CL>
    struct formatter<RS::Graphics::Colour<VT, CS, CL>>:
    formatter<VT> {
        template <typename FormatContext>
        auto format(const RS::Graphics::Colour<VT, CS, CL>& c, FormatContext& ctx) const {
            const std::formatter<VT>& vt_format = *this;
            auto out = ctx.out();
            auto ch = '[';
            for (auto x: c) {
                *out++ = ch;
                out = vt_format.format(x, ctx);
                ch = ',';
            }
            *out++ = ']';
            return out;
        }
    };

    template <RS::Graphics::ColourChannel VT, RS::Graphics::ColourSpace CS, RS::Graphics::ColourLayout CL>
    struct hash<RS::Graphics::Colour<VT, CS, CL>> {
        std::size_t operator()(const RS::Graphics::Colour<VT, CS, CL>& c) const noexcept {
            return c.hash();
        }
    };

}
