#ifndef SNAKE_UTIL_HPP_INCLUDED
#define SNAKE_UTIL_HPP_INCLUDED
//
// util.hpp
//
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <SFML/Graphics.hpp>

//

constexpr std::size_t operator"" _st(unsigned long long number)
{
    return static_cast<std::size_t>(number);
}

constexpr std::ptrdiff_t operator"" _pd(unsigned long long number)
{
    return static_cast<std::ptrdiff_t>(number);
}

//

namespace util
{
    //
    // abs(), min(a,b,c), max(a,b,c)
    //
    // These functions are written here by hand instead of using std because:
    //  * std min/max cannot take more than one arg
    //  * <cmath>'s abs() does not use templates
    //  * some of the std functions are not constexpr when they could be
    //  * some of the std functions are not noexcept when they could be
    //  * comparing reals is only needs to be simple/fast/less-accurate for games
    template <typename T>
    [[nodiscard]] T constexpr abs(const T number) noexcept
    {
        static_assert(std::is_arithmetic_v<T>);

        if constexpr (std::is_unsigned_v<T>)
        {
            return number;
        }
        else
        {
            if (number < T(0))
            {
                return -number;
            }
            else
            {
                return number;
            }
        }
    }

    //

    template <typename T>
    [[nodiscard]] constexpr T max(const T left, const T right) noexcept
    {
        static_assert(std::is_arithmetic_v<T>);

        if (left < right)
        {
            return right;
        }
        else
        {
            return left;
        }
    }

    template <typename T, typename... Ts>
    [[nodiscard]] constexpr T max(const T first, const Ts... allOthers) noexcept
    {
        return max(first, max(allOthers...));
    }

    template <typename T>
    [[nodiscard]] constexpr T min(const T left, const T right) noexcept
    {
        static_assert(std::is_arithmetic_v<T>);

        if (left < right)
        {
            return left;
        }
        else
        {
            return right;
        }
    }

    template <typename T, typename... Ts>
    [[nodiscard]] constexpr T min(const T first, const Ts... allOthers) noexcept
    {
        return min(first, min(allOthers...));
    }

    // this lib is for simple/innaccurate/game/etc apps, so a simple multiple of epsilon works
    template <typename T>
    constexpr T float_compare_epsilon = (std::numeric_limits<T>::epsilon() * T(10));

    //
    // isRealClose()
    //

    template <typename T>
    [[nodiscard]] constexpr bool isRealClose(const T left, const T right) noexcept
    {
        static_assert(std::is_arithmetic_v<T>);

        if constexpr (std::is_integral_v<T>)
        {
            return (left == right);
        }
        else
        {
            const T diffAbs{ abs(right - left) };

            if (diffAbs < T(1))
            {
                return (diffAbs < float_compare_epsilon<T>);
            }
            else
            {
                const T maxForEpsilon{ max(abs(left), abs(right), T(1)) };
                return (diffAbs < (maxForEpsilon * float_compare_epsilon<T>));
            }
        }
    }

    template <typename T>
    [[nodiscard]] constexpr bool isRealCloseOrLess(const T number, const T comparedTo) noexcept
    {
        return ((number < comparedTo) || isRealClose(number, comparedTo));
    }

    template <typename T>
    [[nodiscard]] constexpr bool isRealCloseOrGreater(const T number, const T comparedTo) noexcept
    {
        return ((number > comparedTo) || isRealClose(number, comparedTo));
    }

    //
    // map()
    //

    template <typename T, typename U = T>
    [[nodiscard]] constexpr U
        map(const T number, const T inMin, const T inMax, const U outMin, const U outMax) noexcept
    {
        if (isRealClose(inMin, inMax))
        {
            return outMax;
        }

        return (outMin + static_cast<U>(((number - inMin) * (outMax - outMin)) / (inMax - inMin)));
    }

    // assumes ratio is [0,1]
    template <typename Ratio_t, typename Number_t>
    [[nodiscard]] constexpr Number_t
        mapRatioTo(const Ratio_t ratio, const Number_t outMin, const Number_t outMax) noexcept
    {
        static_assert(std::is_arithmetic_v<Number_t>);
        static_assert(std::is_floating_point_v<Ratio_t>);

        return (
            outMin + static_cast<Number_t>(
                         ratio * (static_cast<Ratio_t>(outMax) - static_cast<Ratio_t>(outMin))));
    }

    template <typename Number_t, typename Ratio_t = float>
    [[nodiscard]] constexpr Ratio_t
        mapToRatio(const Number_t number, const Number_t inMin, const Number_t inMax) noexcept
    {
        static_assert(std::is_floating_point_v<Ratio_t>);

        if (isRealClose(inMin, inMax))
        {
            return Ratio_t(1);
        }

        return static_cast<Ratio_t>((number - inMin) / (inMax - inMin));
    }

    inline constexpr sf::Uint8 mapRatioToColorValue(const float ratio)
    {
        return map(std::clamp(ratio, 0.0f, 1.0f), 0.0f, 1.0f, sf::Uint8(0), sf::Uint8(255));
    }

    constexpr std::size_t verts_per_quad{ 4 };

    template <typename Container_t>
    [[nodiscard]] inline std::string containerToString(
        const Container_t & container,
        const std::string & separator = ",",
        const std::string & wrap = {})
    {
        std::ostringstream ss;

        const auto iterBegin{ std::begin(container) };
        for (auto iter(iterBegin); std::end(container) != iter; ++iter)
        {
            if (iterBegin != iter)
            {
                ss << separator;
            }

            ss << *iter;
        }

        const std::string content{ ss.str() };

        if (content.empty())
        {
            return "";
        }
        else
        {
            const std::string wrapFront{ (wrap.size() >= 1) ? std::string(1, wrap[0])
                                                            : std::string() };

            const std::string wrapBack{ (wrap.size() >= 2) ? std::string(1, wrap[1])
                                                           : std::string() };

            return (wrapFront + content + wrapBack);
        }
    };

    [[nodiscard]] inline const std::string colorToString(const sf::Color & C)
    {
        std::string str;
        str.reserve(16);

        str += '(';

        if (sf::Color::Black == C)
        {
            str += "Black";
        }
        else if (sf::Color::White == C)
        {
            str += "White";
        }
        else if (sf::Color::Red == C)
        {
            str += "Red";
        }
        else if (sf::Color::Green == C)
        {
            str += "Green";
        }
        else if (sf::Color::Blue == C)
        {
            str += "Blue";
        }
        else if (sf::Color::Yellow == C)
        {
            str += "Yellow";
        }
        else if (sf::Color::Magenta == C)
        {
            str += "Magenta";
        }
        else if (sf::Color::Cyan == C)
        {
            str += "Cyan";
        }
        else
        {
            str += std::to_string(static_cast<unsigned>(C.r));
            str += ',';
            str += std::to_string(static_cast<unsigned>(C.g));
            str += ',';
            str += std::to_string(static_cast<unsigned>(C.b));

            if (C.a != 255)
            {
                str += ',';
                str += std::to_string(static_cast<unsigned>(C.a));
            }
        }

        str += ')';

        return str;
    }
} // namespace util

//

namespace sf
{
    using Vector2s = sf::Vector2<std::size_t>;

    template <typename T>
    [[nodiscard]] bool operator<(const sf::Vector2<T> & left, const sf::Vector2<T> & right)
    {
        if (left.x != right.x)
        {
            return (left.x < right.x);
        }
        else
        {
            return (left.y < right.y);
        }
    }

    template <typename T>
    [[nodiscard]] bool operator<=(const sf::Vector2<T> & left, const sf::Vector2<T> & right)
    {
        return ((left == right) || (left < right));
    }

    template <typename T>
    [[nodiscard]] sf::Vector2<T>
        operator*(const sf::Vector2<T> & left, const sf::Vector2<T> & right)
    {
        return { (left.x * right.x), (left.y * right.y) };
    }

    template <typename T>
    [[nodiscard]] sf::Vector2<T>
        operator/(const sf::Vector2<T> & numerator, const sf::Vector2<T> & denominator)
    {
        return { (numerator.x / denominator.x), (numerator.y / denominator.y) };
    }

    //

    template <typename T>
    [[nodiscard]] bool operator<(const sf::Rect<T> & r1, const sf::Rect<T> & r2)
    {
        return (
            std::tie(r1.top, r1.left, r1.width, r1.height) <
            std::tie(r2.top, r2.left, r2.width, r2.height));
    }

    //

    template <typename T>
    std::ostream & operator<<(std::ostream & os, const sf::Vector2<T> & vec)
    {
        os << '(' << vec.x << 'x' << vec.y << ')';
        return os;
    }

    template <typename T>
    std::ostream & operator<<(std::ostream & os, const sf::Rect<T> & rect)
    {
        os << '(' << rect.left << ',' << rect.top << '/' << rect.width << 'x' << rect.height << ')';
        return os;
    }

    inline std::ostream & operator<<(std::ostream & os, const sf::Color & C)
    {
        os << util::colorToString(C);
        return os;
    }

    inline std::ostream & operator<<(std::ostream & os, const sf::Vertex & vert)
    {
        os << "(pos=" << vert.position << ", col=" << vert.color << ", tc=" << vert.texCoords
           << ")";

        return os;
    }

    inline std::ostream & operator<<(std::ostream & os, const sf::VideoMode & vm)
    {
        os << "(" << vm.width << "x" << vm.height << ":" << vm.bitsPerPixel << "bpp";

        if (!vm.isValid())
        {
            os << "(sfml says this mode is invalid)";
        }

        os << ")";

        return os;
    }
} // namespace sf

//

namespace util
{
    template <typename T>
    void sortThenUnique(T & container)
    {
        std::sort(std::begin(container), std::end(container));

        container.erase(
            std::unique(std::begin(container), std::end(container)), std::end(container));
    }

    [[nodiscard]] inline std::string makeSupportedVideoModesString(
        const bool willSkipDiffBitsPerPixel = false, const std::string & separator = "\n")
    {
        const unsigned int desktopBitsPerPixel{ sf::VideoMode::getDesktopMode().bitsPerPixel };

        std::vector<sf::VideoMode> videoModes{ sf::VideoMode::getFullscreenModes() };
        std::reverse(std::begin(videoModes), std::end(videoModes));

        const std::size_t modeCountOrig{ videoModes.size() };

        std::size_t count{ 0 };
        std::ostringstream ss;
        for (const sf::VideoMode & vm : videoModes)
        {
            if (willSkipDiffBitsPerPixel && (vm.bitsPerPixel != desktopBitsPerPixel))
            {
                continue;
            }

            if (count > 0)
            {
                ss << separator;
            }

            ss << vm;
            ++count;
        }

        const std::size_t modeCountReturned{ count };

        ss << separator << "(total_supported=" << modeCountOrig << ")";
        ss << separator << "(total_listed=" << modeCountReturned << ")";

        return ss.str();
    }

    // bit hacking

    template <typename T, typename U>
    static constexpr bool isBitSet(const T bits, const U toCheck) noexcept
    {
        static_assert(
            (std::is_unsigned_v<T>) ||
            (std::is_enum_v<T> && std::is_unsigned_v<typename std::underlying_type_t<T>>));

        static_assert(!std::is_same_v<std::remove_cv<T>, bool>);

        static_assert(std::is_arithmetic_v<U>);
        static_assert(!std::is_same_v<std::remove_cv<U>, bool>);

        if (toCheck < 0)
        {
            return false;
        }

        return ((bits & static_cast<T>(toCheck)) != 0);
    }

    template <typename T, typename U>
    static constexpr void setBit(T & bits, const U toSet) noexcept
    {
        static_assert(
            (std::is_unsigned_v<T>) ||
            (std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>));

        static_assert(!std::is_same_v<std::remove_cv<T>, bool>);

        static_assert(std::is_arithmetic_v<U>);
        static_assert(!std::is_same_v<std::remove_cv<U>, bool>);

        if (toSet < 0)
        {
            return;
        }

        bits |= static_cast<T>(toSet);
    }

    template <typename T, typename U>
    static constexpr T setBitCopy(const T & bits, const U toSet) noexcept
    {
        T copy{ bits };
        setBit(copy, toSet);
        return copy;
    }

    // Counting High Bits
    //  Peter Wegner's Method, which was also discovered independently by Derrick Lehmer in 1964.
    //  This method goes through as many iterations as there are set bits.
    template <typename T>
    [[nodiscard]] std::size_t countHighBits(T number) noexcept
    {
        static_assert(std::is_unsigned_v<T>);
        static_assert(!std::is_same_v<std::remove_cv<T>, bool>);

        std::size_t count{ 0 };
        for (; number; count++)
        {
            number &= (number - 1);
        }

        return count;
    }

    // std lib

    // requires random access but a quick way to erase back without invalidating other iters
    template <typename Container_t>
    void swapAndPop(Container_t & container, const typename Container_t::iterator & toErase)
    {
        if (container.empty())
        {
            return;
        }

        if (container.size() > 1)
        {
            std::iter_swap(toErase, (std::end(container) - 1));
        }

        container.pop_back();
    }

    // math

    constexpr float pi{ 3.1415926535897932f };

    [[nodiscard]] constexpr float degreesToRadians(const float degrees) noexcept
    {
        return (degrees * (pi / 180.0f));
    }

    [[nodiscard]] constexpr float radiansToDegrees(const float radians) noexcept
    {
        return (radians * (180.0f / pi));
    }

    constexpr float tiny{ 0.0001f };

    [[nodiscard]] inline bool isAbsTiny(const float value) noexcept
    {
        return (std::abs(value) < tiny);
    }

    //

    template <typename T>
    [[nodiscard]] sf::Rect<T> floor(const sf::Rect<T> & rect)
    {
        return { std::floor(rect.left),
                 std::floor(rect.top),
                 std::floor(rect.width),
                 std::floor(rect.height) };
    }

    template <typename T>
    [[nodiscard]] sf::Vector2<T> floor(const sf::Vector2<T> & vec)
    {
        return { std::floor(vec.x), std::floor(vec.y) };
    }

    // position, size, and center

    template <typename T>
    [[nodiscard]] sf::Vector2<T> position(const sf::Rect<T> & rect)
    {
        return { rect.left, rect.top };
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f position(const T & thing)
    {
        return position(thing.getGlobalBounds());
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f positionLocal(const T & thing)
    {
        return position(thing.getLocalBounds());
    }

    template <typename T>
    [[nodiscard]] T right(const sf::Rect<T> & rect)
    {
        return (rect.left + rect.width);
    }

    template <typename T>
    [[nodiscard]] float right(const T & thing)
    {
        return right(thing.getGlobalBounds());
    }

    template <typename T>
    [[nodiscard]] T bottom(const sf::Rect<T> & rect)
    {
        return (rect.top + rect.height);
    }

    template <typename T>
    [[nodiscard]] float bottom(const T & thing)
    {
        return bottom(thing.getGlobalBounds());
    }

    template <typename T>
    [[nodiscard]] sf::Vector2<T> size(const sf::Rect<T> & rect)
    {
        return { rect.width, rect.height };
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f size(const T & thing)
    {
        return size(thing.getGlobalBounds());
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f sizeLocal(const T & thing)
    {
        return size(thing.getLocalBounds());
    }

    template <typename T>
    [[nodiscard]] sf::Vector2<T> center(const sf::Rect<T> & rect)
    {
        return (position(rect) + (size(rect) / T(2)));
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f center(const T & thing)
    {
        return center(thing.getGlobalBounds());
    }

    template <typename T>
    [[nodiscard]] sf::Vector2f centerLocal(const T & thing)
    {
        return center(thing.getLocalBounds());
    }

    template <typename T>
    void setOriginToCenter(T & thing)
    {
        thing.setOrigin(centerLocal(thing));
    }

    // sf::Text needs correction after changing the: string, scale, or characterSize
    template <typename T>
    void setOriginToPosition(T & thing)
    {
        thing.setOrigin(positionLocal(thing));
    }

    template <typename T>
    void makeEven(T number, const bool willAdd)
    {
        static_assert(std::is_integral_v<T>);

        if ((number % 2) != 0)
        {
            if (willAdd)
            {
                ++number;
            }
            else
            {
                --number;
            }
        }
    }

    template <typename T>
    T makeEvenCopy(const T number, const bool willAdd)
    {
        static_assert(std::is_integral_v<T>);
        T copy{ number };
        makeEven(copy, willAdd);
        return copy;
    }

    // template <typename Output_t, typename Input_t>
    // Output_t makeMultOf(const Input_t startingNumber, const Output_t mult, const bool willAdd)
    //{
    //    static_assert(std::is_integral_v<Output_t>);
    //
    //    Output_t result{ static_cast<Output_t>(startingNumber) };
    //
    //    while ((result % mult) != 0)
    //    {
    //        if (willAdd)
    //        {
    //            ++result;
    //        }
    //        else
    //        {
    //            if (result > 2)
    //            {
    //                --result;
    //            }
    //            else
    //            {
    //                result = 2;
    //                break;
    //            }
    //        }
    //    }
    //
    //    return result;
    //};

    template <typename Output_t, typename Input_t>
    sf::Vector2<Output_t>
        makeVector2MultOf(const sf::Vector2<Input_t> & before, const sf::Vector2<Output_t> & mults)
    {
        static_assert(std::is_integral_v<Output_t>);
        return { makeMultOf(before.x, mults.x), makeMultOf(before.y, mults.y) };
    };

    // vetor and euclidian math

    [[nodiscard]] inline float
        dotProduct(const sf::Vector2f & left, const sf::Vector2f & right) noexcept
    {
        return static_cast<float>((left.x * right.x) + (left.y * right.y));
    }

    [[nodiscard]] inline sf::Vector2f
        difference(const sf::Vector2f & from, const sf::Vector2f & to) noexcept
    {
        return (to - from);
    }

    [[nodiscard]] inline float magnitude(const sf::Vector2f & vec) noexcept
    {
        return std::sqrtf((vec.x * vec.x) + (vec.y * vec.y));
    }

    [[nodiscard]] inline float distance(const sf::Vector2f & from, const sf::Vector2f & to) noexcept
    {
        return magnitude(to - from);
    }

    [[nodiscard]] inline sf::Vector2f normalize(
        const sf::Vector2f & vec, const sf::Vector2f & returnOnError = { 0.0f, 0.0f }) noexcept
    {
        const float mag{ magnitude(vec) };

        if (mag < tiny)
        {
            return returnOnError;
        }

        return (vec / mag);
    }

    [[nodiscard]] inline sf::Vector2f diffNormal(
        const sf::Vector2f & from,
        const sf::Vector2f & to,
        const sf::Vector2f & returnOnError = { 0.0f, 0.0f }) noexcept
    {
        return normalize(difference(from, to), returnOnError);
    }

    // degrees, assumes 0/360 degrees aims right, and positive degress turns clockwise
    [[nodiscard]] inline float angleFromVector(const sf::Vector2f & velocity)
    {
        const sf::Vector2f posDiffNormal{ normalize(velocity) };
        const float angleRadians{ std::acosf(posDiffNormal.x) };
        const float angleDegrees{ radiansToDegrees(angleRadians) };

        // vertical or Y values that are positive move down, so have to flip
        if (velocity.y < 0.0f)
        {
            return -angleDegrees;
        }
        else
        {
            return angleDegrees;
        }
    }

    // assumes 0 and 360 degrees aims right, and positive degress turns clockwise
    [[nodiscard]] inline float angleFromTo(const sf::Vector2f & from, const sf::Vector2f & to)
    {
        return angleFromVector(difference(from, to));
    }

    template <typename T, typename U = T>
    [[nodiscard]] float angleFromTo(const T & from, const U & to)
    {
        sf::Vector2f fromPos{ 0.0f, 0.0f };
        if constexpr (std::is_same_v<std::remove_cv_t<T>, sf::Vector2f>)
        {
            fromPos = from;
        }
        else
        {
            fromPos = center(from);
        }

        sf::Vector2f toPos{ 0.0f, 0.0f };
        if constexpr (std::is_same_v<std::remove_cv_t<U>, sf::Vector2f>)
        {
            toPos = to;
        }
        else
        {
            toPos = center(to);
        }

        return angleFromTo(fromPos, toPos);
    }

    template <typename T>
    void aimAtPosition(T & thing, const sf::Vector2f & pos)
    {
        thing.setRotation(angleFromTo(center(thing), pos));
    }

    template <typename T>
    void aimWithVector(T & thing, const sf::Vector2f & velocity)
    {
        thing.setRotation(angleFromVector(velocity));
    }

    // scales, offsets, and local bounds

    // sfml utils to re-size (scale) any sf::FloatRect without moving it
    inline void scaleRectInPlace(sf::FloatRect & rect, const sf::Vector2f & scale) noexcept
    {
        const auto widthChange((rect.width * scale.x) - rect.width);
        rect.width += widthChange;
        rect.left -= (widthChange * 0.5f);

        const float heightChange((rect.height * scale.y) - rect.height);
        rect.height += heightChange;
        rect.top -= (heightChange * 0.5f);
    }

    [[nodiscard]] inline sf::FloatRect
        scaleRectInPlaceCopy(const sf::FloatRect & before, const sf::Vector2f & scale) noexcept
    {
        sf::FloatRect after(before);
        scaleRectInPlace(after, scale);
        return after;
    }

    inline void scaleRectInPlace(sf::FloatRect & rect, const float scale) noexcept
    {
        scaleRectInPlace(rect, { scale, scale });
    }

    [[nodiscard]] inline sf::FloatRect
        scaleRectInPlaceCopy(const sf::FloatRect & before, const float scale) noexcept
    {
        sf::FloatRect after(before);
        scaleRectInPlace(after, scale);
        return after;
    }

    inline void adjRectInPlace(sf::FloatRect & rect, const float amount) noexcept
    {
        rect.left += amount;
        rect.top += amount;
        rect.width -= (amount * 2.0f);
        rect.height -= (amount * 2.0f);
    }

    [[nodiscard]] inline sf::FloatRect
        adjRectInPlaceCopy(const sf::FloatRect & before, const float amount) noexcept
    {
        sf::FloatRect after(before);
        adjRectInPlace(after, amount);
        return after;
    }

    // re-sizing (scaling), centering, and all while maintaining origins

    // without changing the shape
    template <typename T>
    void fit(T & thing, const sf::Vector2f & size)
    {
        // skip if source size is zero (or close) to avoid dividing by zero below
        const sf::FloatRect localBounds{ thing.getLocalBounds() };
        if ((localBounds.width < 1.0f) || (localBounds.height < 1.0f))
        {
            return;
        }

        const float scaleHoriz{ size.x / localBounds.width };
        thing.setScale(scaleHoriz, scaleHoriz);

        if (thing.getGlobalBounds().height > size.y)
        {
            const float scaleVert{ size.y / localBounds.height };
            thing.setScale(scaleVert, scaleVert);
        }

        if constexpr (std::is_same_v<std::remove_cv_t<T>, sf::Text>)
        {
            setOriginToPosition(thing);
        }
    }

    template <typename T>
    void fit(T & thing, const sf::FloatRect & rect)
    {
        fit(thing, { rect.width, rect.height });
    }

    template <typename T>
    void fit(T & thing, const float newScale)
    {
        fit(thing, { newScale, newScale });
    }

    template <typename T>
    void centerInside(T & thing, const sf::FloatRect & rect)
    {
        thing.setPosition((center(rect) - (size(thing) * 0.5f)) + thing.getOrigin());
    }

    template <typename T>
    void fitAndCenterInside(T & thing, const sf::FloatRect & rect)
    {
        fit(thing, rect);
        centerInside(thing, rect);
    }

    // quad making and appending

    template <typename Container_t>
    void setupQuadVerts(
        const sf::Vector2f & pos,
        const sf::Vector2f & size,
        const std::size_t index,
        Container_t & verts,
        const sf::Color & color = sf::Color::Transparent)
    {
        // clang-format off
        verts[index + 0].position = pos;
        verts[index + 1].position = sf::Vector2f((pos.x + size.x),  pos.y          );
        verts[index + 2].position = sf::Vector2f((pos.x + size.x), (pos.y + size.y));
        verts[index + 3].position = sf::Vector2f( pos.x          , (pos.y + size.y));
        // clang-format on

        if (color != sf::Color::Transparent)
        {
            verts[index + 0].color = color;
            verts[index + 1].color = color;
            verts[index + 2].color = color;
            verts[index + 3].color = color;
        }
    }

    template <typename Container_t>
    void setupQuadVerts(
        const sf::FloatRect & rect,
        const std::size_t index,
        Container_t & verts,
        const sf::Color & color = sf::Color::Transparent)
    {
        setupQuadVerts(position(rect), size(rect), index, verts, color);
    }

    template <typename Container_t>
    void appendQuadVerts(
        const sf::Vector2f & pos,
        const sf::Vector2f & size,
        Container_t & verts,
        const sf::Color & color = sf::Color::Transparent)
    {
        std::size_t origSize{ 0 };
        if constexpr (std::is_same_v<std::remove_cv_t<Container_t>, sf::VertexArray>)
        {
            origSize = verts.getVertexCount();
        }
        else
        {
            origSize = verts.size();
        }

        verts.resize(origSize + 4);

        setupQuadVerts(pos, size, origSize, verts, color);
    }

    template <typename Container_t>
    void appendQuadVerts(
        const sf::FloatRect & rect,
        Container_t & verts,
        const sf::Color & color = sf::Color::Transparent)
    {
        appendQuadVerts(position(rect), size(rect), verts, color);
    }

    // slow running but handy debugging shapes

    [[nodiscard]] inline sf::VertexArray
        makeRectangleVerts(const sf::FloatRect & rect, const sf::Color & color = sf::Color::White)
    {
        sf::VertexArray verts(sf::Quads, 4);
        setupQuadVerts(position(rect), size(rect), 0, verts, color);
        return verts;
    }

    inline void drawRectangleVerts(
        sf::RenderTarget & target,
        const sf::FloatRect & rect,
        const sf::Color & color = sf::Color::White)
    {
        target.draw(makeRectangleVerts(rect, color));
    }

    [[nodiscard]] inline sf::RectangleShape makeRectangleShape(
        const sf::FloatRect & rect,
        const bool willColorFill = false,
        const sf::Color & color = sf::Color::White)
    {
        sf::RectangleShape rs;

        rs.setOutlineThickness(1.0f);
        rs.setOutlineColor(color);

        if (willColorFill)
        {
            rs.setFillColor(color);
        }
        else
        {
            rs.setFillColor(sf::Color::Transparent);
        }

        rs.setPosition(position(rect));
        rs.setSize(size(rect));
        return rs;
    }

    inline void drawRectangleShape(
        sf::RenderTarget & target,
        const sf::FloatRect & rect,
        const bool willColorFill = false,
        const sf::Color & color = sf::Color::White)
    {
        target.draw(makeRectangleShape(rect, willColorFill, color));
    }

    [[nodiscard]] inline sf::CircleShape makeCircleShape(
        const sf::Vector2f & position,
        const float radius,
        const sf::Color & color = sf::Color::White,
        const std::size_t pointCount = 32)
    {
        sf::CircleShape cs;
        cs.setFillColor(color);
        cs.setPointCount(pointCount);
        cs.setRadius(radius);
        setOriginToCenter(cs);
        cs.setPosition(position);
        return cs;
    }

    inline void drawCircleShape(
        sf::RenderTarget & target,
        const sf::Vector2f & position,
        const float radius,
        const sf::Color & color = sf::Color::White,
        const std::size_t pointCount = 32)
    {
        target.draw(makeCircleShape(position, radius, color, pointCount));
    }

    [[nodiscard]] inline sf::CircleShape makeCircleShape(
        const sf::FloatRect & rect,
        const sf::Color & color = sf::Color::White,
        const std::size_t pointCount = 32)
    {
        return makeCircleShape(
            center(rect), (std::min(rect.width, rect.height) * 0.5f), color, pointCount);
    }

    inline void drawCircle(
        sf::RenderTarget & target,
        const sf::FloatRect & rect,
        const sf::Color & color = sf::Color::White)
    {
        target.draw(makeCircleShape(rect, color));
    }

    inline sf::VertexArray makeLines(
        const std::vector<sf::Vector2f> & points, const sf::Color & color = sf::Color::White)
    {
        sf::VertexArray va(sf::Lines);

        for (const sf::Vector2f & point : points)
        {
            va.append(sf::Vertex(point, color));
        }

        return va;
    }

    inline sf::VertexArray makeLines(
        const std::initializer_list<sf::Vector2f> & initListPoints,
        const sf::Color & color = sf::Color::White)
    {
        std::vector<sf::Vector2f> points;
        points.reserve(initListPoints.size());

        for (const sf::Vector2f & point : points)
        {
            points.push_back(point);
        }

        return makeLines(points, color);
    }

    inline void drawlines(
        sf::RenderTarget & target,
        const std::vector<sf::Vector2f> & points,
        const sf::Color & color = sf::Color::White)
    {
        target.draw(makeLines(points, color));
    }

    inline void drawlines(
        sf::RenderTarget & target,
        const std::initializer_list<sf::Vector2f> & points,
        const sf::Color & color = sf::Color::White)
    {
        target.draw(makeLines(points, color));
    }

    // more misc sfml

    inline sf::Color colorBlend(
        const float ratio,
        const sf::Color & fromColor,
        const sf::Color & toColor,
        const bool willIgnoreAlpha = false)
    {
        if (ratio < 0.0f)
        {
            return fromColor;
        }

        if (ratio > 1.0f)
        {
            return toColor;
        }

        auto calcColorValue = [ratio](const sf::Uint8 fromVal, const sf::Uint8 toVal) {
            const float diff{ static_cast<float>(toVal) - static_cast<float>(fromVal) };
            const float finalValue{ static_cast<float>(fromVal) + (diff * ratio) };
            return static_cast<sf::Uint8>(finalValue);
        };

        sf::Color color{ toColor };
        color.r = calcColorValue(fromColor.r, toColor.r);
        color.g = calcColorValue(fromColor.g, toColor.g);
        color.b = calcColorValue(fromColor.b, toColor.b);

        if (!willIgnoreAlpha)
        {
            color.a = calcColorValue(fromColor.a, toColor.a);
        }

        return color;
    }

    inline sf::Color colorStepToward(
        const sf::Uint8 stepSize,
        const sf::Color & fromColor,
        const sf::Color & toColor,
        const bool willIgnoreAlpha = false)
    {
        if (0 == stepSize)
        {
            return fromColor;
        }

        if (255 == stepSize)
        {
            return toColor;
        }

        auto calcColorValue = [stepSize](const sf::Uint8 fromVal, const sf::Uint8 toVal) {
            if (fromVal == toVal)
            {
                return fromVal;
            }

            const int stepInt{ static_cast<int>(stepSize) };
            const int fromInt{ static_cast<int>(fromVal) };
            const int toInt{ static_cast<int>(toVal) };
            const int diff{ std::min(std::abs(toInt - fromInt), stepInt) };

            int finalValue{ fromInt };
            if (toVal > fromVal)
            {
                finalValue += diff;
            }
            else
            {
                finalValue -= diff;
            }

            return static_cast<sf::Uint8>(std::clamp(finalValue, 0, 255));
        };

        sf::Color color{ toColor };
        color.r = calcColorValue(fromColor.r, toColor.r);
        color.g = calcColorValue(fromColor.g, toColor.g);
        color.b = calcColorValue(fromColor.b, toColor.b);

        if (!willIgnoreAlpha)
        {
            color.a = calcColorValue(fromColor.a, toColor.a);
        }

        return color;
    }

    // statistics

    template <typename T>
    struct Stats
    {
        std::string toString(const std::streamsize numberWidth = 5) const
        {
            std::ostringstream ss;
            ss.imbue(std::locale("")); // this is only to put commas in the big numbers

            ss << "x" << count;
            ss << " [" << std::setw(numberWidth) << std::right << min;
            ss << ", " << std::setw(numberWidth) << std::right << static_cast<T>(avg);
            ss << ", " << std::setw(numberWidth) << std::right << max;
            ss << "] sd=" << std::setw(numberWidth) << std::left << sdv;

            return ss.str();
        }

        std::size_t count{ 0 };
        T min{ T(0) };
        T max{ T(0) };
        T sum{ T(0) };
        double avg{ 0.0 };
        double sdv{ 0.0 };
    };

    template <typename T>
    std::ostream & operator<<(std::ostream & os, const Stats<T> & stats)
    {
        os << stats.toString();
        return os;
    }

    template <typename Container_t>
    Stats<typename Container_t::value_type> makeStats(const Container_t & container)
    {
        using T = typename Container_t::value_type;

        Stats<T> stats;

        stats.count = container.size();

        stats.min = std::numeric_limits<T>::max();

        for (const T number : container)
        {
            stats.sum += number;

            if (number < stats.min)
            {
                stats.min = number;
            }

            if (number > stats.max)
            {
                stats.max = number;
            }
        }

        stats.avg = (static_cast<double>(stats.sum) / static_cast<double>(stats.count));

        if (stats.count < 2)
        {
            return stats;
        }

        double deviationSum{ 0.0 };
        for (const T number : container)
        {
            const double diff{ static_cast<double>(number) - stats.avg };
            deviationSum += (diff * diff);
        }

        stats.sdv = std::sqrt(deviationSum / static_cast<double>(stats.count));

        return stats;
    }

    // media directory finding

    struct MediaPath
    {
        explicit MediaPath(const std::filesystem::path & pathParam)
            : path(pathParam)
            , error_message(setErrorMessage(pathParam))
        {}

        bool isValid() const { return error_message.empty(); }

        static std::string setErrorMessage(const std::filesystem::path & path)
        {
            if (!std::filesystem::exists(path))
            {
                return "Directory was not found.";
            }
            else if (!std::filesystem::is_directory(path))
            {
                return "Path is not a directory.";
            }
            else
            {
                return "";
            }
        }

        std::filesystem::path path;
        std::string error_message;
    };

    // percent stuff

    template <typename T, typename U = T>
    [[nodiscard]] float calcPercent(const T num, const U den, const std::size_t afterDotCount = 1)
    {
        static_assert(std::is_arithmetic_v<T>);
        static_assert(!std::is_same_v<std::remove_cv_t<T>, bool>);

        static_assert(std::is_arithmetic_v<U>);
        static_assert(!std::is_same_v<std::remove_cv_t<U>, bool>);

        if (!(den > U{ 0 }))
        {
            return 0.0f;
        }

        long double result{ (static_cast<long double>(num) / static_cast<long double>(den)) };
        result *= 100.0L;

        if (afterDotCount > 0)
        {
            const long double afterDotMult{ 10.0L * static_cast<long double>(afterDotCount) };
            if (afterDotMult > 0.0L)
            {
                result *= afterDotMult;
                result = std::round(result);
                result /= afterDotMult;
            }
        }

        return static_cast<float>(result);
    }

    template <typename T, typename U = T>
    [[nodiscard]] std::string makePercentString(
        const T num,
        const U den,
        const std::string & prefix = {},
        const std::string & postfix = {},
        const std::size_t afterDotCount = 1,
        const std::string & wrap = "()")
    {
        std::ostringstream ss;

        if (!wrap.empty())
        {
            ss << wrap.front();
        }

        ss << prefix;
        ss << calcPercent<T, U>(num, den, afterDotCount);
        ss << '%';
        ss << postfix;

        if (!wrap.empty())
        {
            ss << wrap.back();
        }

        return ss.str();
    }
} // namespace util

#endif // SNAKE_UTIL_HPP_INCLUDED
