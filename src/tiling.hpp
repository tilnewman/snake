#ifndef SNAKE_TILING_HPP_INCLUDED
#define SNAKE_TILING_HPP_INCLUDED
//
// tiling.hpp
//
/*
#include "check-macros.hpp"
#include "common-types.hpp"
#include "util.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <SFML/Graphics/Rect.hpp>

namespace snake::tile
{
    using Feature_t = unsigned;

    namespace Feature
    {
        // clang-format off
        enum : Feature_t
        {
            None            = 0,
            //
            CountCanIncr    = 1 << 0,
            //
            Filled          = 1 << 1,
            Obstacles       = 1 << 2,
            //
            OneRow          = 1 << 3,
            OneColumn       = 1 << 4,
            ColCountSame    = 1 << 5,
            ColRowCountSame = 1 << 6,
            //
            RectsSameArea   = 1 << 7,
            RectsSameShape  = 1 << 8,
            RectsSquare     = 1 << 9
        };
        // clang-format on

        // CountCanIncr
        // OneRow
        // OneColumn
        // ColRowCountSame
        // RectsSquare

        std::string toName(const Feature_t feature);

        std::vector<std::string>
            toStrings(const Feature_t features, const bool willSkipDefaults = false);

        inline std::string toString(
            const Feature_t features,
            const bool willSkipDefaults = false,
            const std::string & separator = ",",
            const std::string & wrap = "()")
        {
            return util::containerToString(toStrings(features, willSkipDefaults), separator, wrap);
        }

        void populateDiffStrings(
            const Feature_t from,
            const Feature_t to,
            std::vector<std::string> & losts,
            std::vector<std::string> & gains);
    } // namespace Feature

    //
    namespace squareroots
    {
        //
        template <typename T>
        inline bool isPerfectSquare(const T number)
        {
            static_assert(std::is_integral_v<T>);

            if (number < 4)
            {
                return false;
            }

            const T squareRootFloor{ static_cast<T>(std::sqrt(number)) };
            return ((squareRootFloor * squareRootFloor) == number);
        }

        //
        template <typename T>
        inline bool isAlmostPerfectSquare(const T number)
        {
            static_assert(std::is_integral_v<T>);

            if (number < 4)
            {
                return false;
            }

            const T squareRootFloor{ static_cast<T>(std::sqrt(number)) };
            return ((squareRootFloor * (squareRootFloor + 1)) == number);
        }

        //
        template <typename T>
        inline bool isSquareEnough(const T number, const bool mustBePerfectSquare)
        {
            if (mustBePerfectSquare)
            {
                return isPerfectSquare(number);
            }
            else
            {
                return isAlmostPerfectSquare(number);
            }
        }

        // may return the number given
        template <typename T>
        inline T findPrevSquare(const T count, const bool mustBePerfectSquare)
        {
            T result{ count };

            if (result < 4)
            {
                return 0;
            }

            while ((result >= 4) && !isSquareEnough(result, mustBePerfectSquare))
            {
                --result;
            }

            return result;
        }

        // may return the number given
        template <typename T>
        inline T findNextSquare(const T count, const bool mustBePerfectSquare)
        {
            const T max{ 100 + (count * 2) + (count * (count / 10)) };

            T result{ count };
            while ((result < max) && !isSquareEnough(result, mustBePerfectSquare))
            {
                ++result;
            }

            if (result >= max)
            {
                return 0;
            }

            return result;
        }
    } // namespace squareroots

    // HOW an area is tiled, but not all Specs are possible on all areas so always check Result
    struct Spec
    {
        constexpr Spec() noexcept = default;

        constexpr explicit Spec(
            const int countParam, const Feature_t featuresParam, const int borderParam = 0) noexcept
        {
            setup(countParam, featuresParam, borderParam);
        }

        constexpr void setup(
            const int countParam, const Feature_t featuresParam, const int borderParam = 0) noexcept
        {
            count = countParam;
            border = ((borderParam < 0) ? -borderParam : borderParam);
            features = featuresParam;
        }

        inline constexpr bool empty() const noexcept { return (count <= 0); }

        inline constexpr bool isValid() const noexcept
        {
            return (!empty() && (isRooms() || isBlocks() || isOneDimm()));
        }

        inline constexpr bool isBlocks() const noexcept
        {
            return ((features & Feature::Obstacles) && (border > 0));
        }

        inline constexpr bool isRooms() const noexcept
        {
            return (!(features & Feature::Obstacles) && (0 == border));
        }

        inline constexpr bool isOneDimm() const noexcept
        {
            if (1 == count)
            {
                return ((features & Feature::OneRow) && (features & Feature::OneColumn));
            }
            else
            {
                return ((features & Feature::OneRow) || (features & Feature::OneColumn));
            }
        }

        std::string toString() const;

        //
        int count{ 0 };
        int border{ 0 };
        Feature_t features{ Feature::None };
    };

    [[nodiscard]] inline constexpr bool operator==(const Spec & left, const Spec & right) noexcept
    {
        return (
            std::tie(left.count, left.features, left.border) ==
            std::tie(right.count, right.features, right.border));
    }

    [[nodiscard]] inline constexpr bool operator!=(const Spec & left, const Spec & right) noexcept
    {
        return !(left == right);
    }

    [[nodiscard]] inline constexpr bool operator<(const Spec & left, const Spec & right) noexcept
    {
        return (
            std::tie(left.count, left.features, left.border) <
            std::tie(right.count, right.features, right.border));
    }

    //
    template <typename T>
    struct Stats
    {
        explicit Stats(
            const std::string & nameParam,
            const std::vector<T> & valuesParam = {},
            const bool willSortParam = true)
            : name(nameParam)
            , values(valuesParam)
            , will_sort(willSortParam)
        {
            mmu_str.reserve(128);
            list_str.reserve(128);

            if (!empty() && (count() >= 0))
            {
                setup();
            }
        }

        inline bool empty() const { return values.empty(); }

        inline int count() const { return static_cast<int>(values.size()); }

        void setup()
        {
            M_CHECK_SS(!name.empty(), "name=\"" << name << "\"");

            M_CHECK_SS(
                (!empty() && (count() >= 0)),
                "name=" << name << ", values.size()=" << values.size() << ", count()=" << count());

            if (will_sort)
            {
                std::sort(std::begin(values), std::end(values));
            }

            mmu_str.clear();
            list_str.clear();

            auto appendToString = [](const T & thing, std::string & str) {
                if constexpr (std::is_arithmetic_v<T>)
                {
                    str += std::to_string(thing);
                }
                else
                {
                    str += '(';
                    str += std::to_string(thing.x);
                    str += 'x';
                    str += std::to_string(thing.y);
                    str += ')';
                }
            };

            if constexpr (std::is_arithmetic_v<T>)
            {
                min = std::numeric_limits<T>::max();
            }
            else
            {
                min = T{ std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
            }

            //
            std::map<T, int> valToCountMap;
            for (const T & val : values)
            {
                sum += val;

                valToCountMap[val]++;

                if (max < val)
                {
                    max = val;
                }

                if (val < min)
                {
                    min = val;
                }
            }

            unique_count = static_cast<int>(valToCountMap.size());

            //
            mmu_str += 'x';
            mmu_str += std::to_string(values.size());
            mmu_str += '[';
            appendToString(min, mmu_str);

            if (values.size() == 1)
            {
                mmu_str += ']';
            }
            else
            {
                mmu_str += ",";
                appendToString(max, mmu_str);

                mmu_str += "],s";
                appendToString(sum, mmu_str);
                mmu_str += ",u";
                mmu_str += std::to_string(unique_count);
            }

            //
            std::multimap<int, T, std::greater<int>> countToValueMap;
            for (const auto & [val, cnt] : valToCountMap)
            {
                countToValueMap.emplace(cnt, val);
            }

            //
            if (will_sort)
            {
                for (const auto & [cnt, val] : countToValueMap)
                {
                    list_str += 'x';
                    list_str += std::to_string(cnt);
                    list_str += ':';
                    appendToString(val, list_str);
                    list_str += ", ";
                }
            }
            else
            {
                for (const T & val : values)
                {
                    appendToString(val, list_str);
                    list_str += ',';
                }
            }

            auto cleanStringEnd = [](std::string & str) {
                while (!str.empty() && ((str.back() == ' ') || (str.back() == ',')))
                {
                    str.pop_back();
                }
            };

            cleanStringEnd(mmu_str);
            cleanStringEnd(list_str);

            validate();
        }

        void reset()
        {
            // don't clear the name
            min = T{};
            max = T{};
            sum = T{};
            unique_count = 0;
            values.clear();
            mmu_str.clear();
            list_str.clear();
        }

        std::string toString(const std::size_t indentParam = 0) const
        {
            std::ostringstream ss;

            const std::size_t indentToUse{ (indentParam == 0) ? 22 : (indentParam + 1) };

            ss << std::setw(12) << std::right << name;
            ss << ' ';
            ss << std::setw(static_cast<int>(indentToUse)) << std::left << mmu_str;
            ss << ' ';
            ss << list_str;

            return ss.str();
        }

        bool isValid() const
        {
            if (values.empty() || (max < min) || (sum < (min * count())) || (sum < max) ||
                mmu_str.empty() || list_str.empty())
            {
                return false;
            }

            if constexpr (std::is_arithmetic_v<T>)
            {
                return { (0 < min) && (0 < max) && (count() <= sum) };
            }
            else
            {
                return { (T{ 0, 0 } < min) && (T{ 0, 0 } < max) && (T(count(), count()) <= sum) };
            }
        }

        //
        std::string name;
        std::vector<T> values;
        T min{ T{} };
        T max{ T{} };
        T sum{ T{} };
        int unique_count{ 0 };
        std::string mmu_str;
        std::string list_str;
        bool will_sort{ true };

      private:
        void validate() const
        {
            M_CHECK_SS(
                (!empty() && (count() >= 0) && !values.empty()),
                "name=" << name << ", values.size()=" << values.size() << ", count()=" << count());

            M_CHECK_SS(
                (unique_count >= 0) && (unique_count <= count()),
                "name=" << name << ", count=" << count() << ", unique_count=" << unique_count);

            M_CHECK_SS((min <= max), "min=" << min << ", max=" << max);

            if (1 == count())
            {
                M_CHECK_SS(
                    (sum == values.front()),
                    "name=" << name << ", sum=" << sum << ", values.front()=" << values.front());

                M_CHECK_SS(
                    ((sum == min) && (sum == max)),
                    "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max);

                M_CHECK_SS((min == max), "name=" << name << ", min=" << min << ", max=" << max);

                M_CHECK_SS(
                    (1 == unique_count), "name=" << name << ", unique_count=" << unique_count);
            }

            if (min == max)
            {
                if (1 == count())
                {
                    M_CHECK_SS(
                        (sum == min),
                        "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max
                                << ", count()=" << count());
                }
                else if (2 == count())
                {
                    M_CHECK_SS(
                        (sum == (min + max)),
                        "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max
                                << ", count()=" << count());
                }
                else
                {
                    M_CHECK_SS(
                        ((min + max) <= sum),
                        "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max
                                << ", count()=" << count());
                }

                if constexpr (std::is_arithmetic_v<T>)
                {
                    if (min > 0)
                    {
                        M_CHECK_SS(
                            ((sum / min) == count()),
                            "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max
                                    << ", count()=" << count());
                    }
                }

                M_CHECK_SS(
                    (1 == unique_count), "name=" << name << ", unique_count=" << unique_count);
            }
            else
            {
                M_CHECK_SS(
                    ((min + max) <= sum),
                    "name=" << name << ", sum=" << sum << ", min=" << min << ", max=" << max);

                M_CHECK_SS((count() >= 2), "name=" << name << ", count()=" << count());
            }

            M_CHECK_SS(!mmu_str.empty(), "mmu_str.size()=" << mmu_str.size());
            M_CHECK_SS(!list_str.empty(), "list_str.size()=" << list_str.size());
        }
    };

    template <typename T>
    [[nodiscard]] inline bool operator==(const Stats<T> & left, const Stats<T> & right)
    {
        return (std::tie(left.name, left.values) == std::tie(right.name, right.values));
    }

    template <typename T>
    [[nodiscard]] inline bool operator!=(const Stats<T> & left, const Stats<T> & right)
    {
        return !(left == right);
    }

    template <typename T>
    [[nodiscard]] inline bool operator<(const Stats<T> & left, const Stats<T> & right)
    {
        return (std::tie(left.name, left.values) < std::tie(right.name, right.values));
    }

    //
    struct Job;

    //
    struct Result
    {
        // intentionally does not check wall_positions, see isValid()
        inline bool empty() const
        {
            return (rects.empty() || columns.empty() || wall_positions.empty());
        }

        int tileCount() const { return columns.sum; }
        int rowCount() const { return columns.count(); }

        const std::pair<int, int> columnCountMinMax() const
        {
            if (columns.isValid())
            {
                return { columns.min, columns.max };
            }
            else if (!rects.empty())
            {
                const auto iterPair{ std::minmax_element(
                    std::begin(rects),
                    std::end(rects),
                    [](const IntRectVec_t & left, const IntRectVec_t & right) {
                        return (left.size() < right.size());
                    }) };

                return { static_cast<int>(iterPair.first->size()),
                         static_cast<int>(iterPair.second->size()) };
            }
            else
            {
                // it is never vaild for a row to have zero columns
                return { 0, 0 };
            }
        }

        inline bool isValid() const { return (!empty() && is_success); }

        void reset();

        void setup(Job & job);

        inline std::string toString() const { return summary_str; }

        sf::IntRect minEnclosingRect() const
        {
            const sf::IntRect topLeftTile{ rects.front().front() };
            const sf::IntRect botRightTile{ rects.back().back() };

            const sf::Vector2i topLeftTilePos(util::position(topLeftTile));

            const sf::Vector2i botRightPosOfBotRightTile(
                util::position(botRightTile) + util::size(botRightTile));

            return sf::IntRect(topLeftTilePos, botRightPosOfBotRightTile);
        }

        sf::Vector2i minEnclosingSize() const { return util::size(minEnclosingRect()); }

        void sort()
        {
            // left-to-right, top-to-bottom
            for (IntRectVec_t & row : rects)
            {
                std::sort(std::begin(row), std::end(row));
            }
        }

        //
        bool is_success{ false };

        bool is_valid_rooms{ false };
        bool is_valid_bocks{ false };
        bool is_valid_one_dimm{ false };

        Feature_t features{ Feature::None };

        Stats<sf::Vector2i> shapes{ "Shapes" };
        Stats<int> areas{ "Areas" };
        Stats<int> hollow_areas{ "Hollows" };
        Stats<int> columns{ "Columns", {}, false };

        IntRectVecVec_t rects;
        snake::BoardPosVec_t wall_positions;

      private:
        std::string diff_str;
        std::string summary_str;

        bool populateAndVerifyStats(const Spec & spec);
        void detectFeatures(const Spec & spec);
        // void verifyStats() const;
        void verifyFeatures(const Spec & spec) const;
        void findMatchingStyles();

        std::string makeSummaryString(const Spec & spec, const sf::IntRect & bounds) const;

        std::string makeDiffString(
            std::vector<std::string> & losts, std::vector<std::string> & gains) const;

        bool determineSuccess(
            const Spec & spec, std::vector<std::string> & losts, std::vector<std::string> & gains);

        //
        friend bool operator==(const Result & left, const Result & right) noexcept;
        friend bool operator<(const Result & left, const Result & right) noexcept;
    };

    [[nodiscard]] inline bool operator==(const Result & left, const Result & right) noexcept
    {
        return (
            std::tie(
                left.is_success,
                left.features,
                left.is_valid_rooms,
                left.is_valid_bocks,
                left.is_valid_one_dimm,
                left.columns,
                left.shapes) ==
            std::tie(
                right.is_success,
                right.features,
                right.is_valid_rooms,
                right.is_valid_bocks,
                right.is_valid_one_dimm,
                right.columns,
                right.shapes));
    }

    [[nodiscard]] inline bool operator!=(const Result & left, const Result & right) noexcept
    {
        return !(left == right);
    }

    [[nodiscard]] inline bool operator<(const Result & left, const Result & right) noexcept
    {
        return (
            std::tie(
                left.is_success,
                left.features,
                left.is_valid_rooms,
                left.is_valid_bocks,
                left.is_valid_one_dimm,
                left.columns,
                left.shapes) <
            std::tie(
                right.is_success,
                right.features,
                right.is_valid_rooms,
                right.is_valid_bocks,
                right.is_valid_one_dimm,
                right.columns,
                right.shapes));
    }

    //
    struct Job
    {
        inline bool isValid() const
        {
            return (
                (bounds.width > 0) && (bounds.height > 0) && spec.isValid() && result.isValid());
        }

        inline std::string toString() const { return result.toString(); }

        sf::IntRect bounds;
        Spec spec;
        Result result;
    };

    [[nodiscard]] inline bool operator==(const Job & left, const Job & right) noexcept
    {
        return (
            std::tie(left.bounds, left.spec, left.result) ==
            std::tie(right.bounds, right.spec, right.result));
    }

    [[nodiscard]] inline bool operator!=(const Job & left, const Job & right) noexcept
    {
        return !(left == right);
    }

    [[nodiscard]] inline bool operator<(const Job & left, const Job & right) noexcept
    {
        return (
            std::tie(left.bounds, left.spec, left.result) <
            std::tie(right.bounds, right.spec, right.result));
    }

    // holds pre-computed tilings for easy later lookup
    struct Warehouse
    {
        void reset() { jobs.clear(); }

        void printSelfTest()
        {
            std::cout << "Warehouse Self Test:";
            std::cout << "\n\t total   = " << jobs.size();

            if (jobs.empty())
            {
                std::cout << " -so bail." << std::endl << std::endl;
                return;
            }

            for (Job & job : jobs)
            {
                if (!job.isValid())
                {
                    std::cout << "\n\t Job::isValid() returned false: " << job.toString();
                }
            }

            {
                std::vector<Job> temp(jobs);
                std::sort(std::begin(temp), std::end(temp));
                const std::size_t countBefore{ temp.size() };
                temp.erase(std::unique(std::begin(temp), std::end(temp)), std::end(temp));
                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t Full-Duplicates = " << (countBefore - countAfter);
            }

            {
                std::vector<Job> temp(jobs);

                std::sort(
                    std::begin(temp), std::end(temp), [&](const Job & left, const Job & right) {
                        return (left.spec < right.spec);
                    });

                const std::size_t countBefore{ temp.size() };

                temp.erase(
                    std::unique(
                        std::begin(temp),
                        std::end(temp),
                        [](const Job & left, const Job & right) {
                            return (left.spec == right.spec);
                        }),
                    std::end(temp));

                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t Spec-Duplicates = " << (countBefore - countAfter);
            }

            {
                std::vector<Job> temp(jobs);

                std::sort(
                    std::begin(temp), std::end(temp), [&](const Job & left, const Job & right) {
                        return (left.spec.features < right.spec.features);
                    });

                const std::size_t countBefore{ temp.size() };

                temp.erase(
                    std::unique(
                        std::begin(temp),
                        std::end(temp),
                        [&](const Job & left, const Job & right) {
                            return (left.spec.features == right.spec.features);
                        }),
                    std::end(temp));

                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t Feature-Duplicates = " << (countBefore - countAfter);
            }

            {
                std::vector<Job> temp(jobs);

                std::sort(
                    std::begin(temp), std::end(temp), [&](const Job & left, const Job & right) {
                        return (left.result.columns < right.result.columns);
                    });

                const std::size_t countBefore{ temp.size() };

                temp.erase(
                    std::unique(
                        std::begin(temp),
                        std::end(temp),
                        [&](const Job & left, const Job & right) {
                            return (left.result.columns == right.result.columns);
                        }),
                    std::end(temp));

                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t Col/Row-Duplicates = " << (countBefore - countAfter);
            }

            {
                std::vector<Job> temp(jobs);

                std::sort(
                    std::begin(temp), std::end(temp), [&](const Job & left, const Job & right) {
                        return (left.result.shapes < right.result.shapes);
                    });

                const std::size_t countBefore{ temp.size() };

                temp.erase(
                    std::unique(
                        std::begin(temp),
                        std::end(temp),
                        [&](const Job & left, const Job & right) {
                            return (left.result.shapes == right.result.shapes);
                        }),
                    std::end(temp));

                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t Shape-Duplicates = " << (countBefore - countAfter);
            }

            {
                std::vector<Job> temp(jobs);

                for (Job & job : temp)
                {
                    std::sort(
                        std::begin(job.result.wall_positions), std::end(job.result.wall_positions));
                }

                std::sort(
                    std::begin(temp), std::end(temp), [&](const Job & left, const Job & right) {
                        return (left.result.wall_positions < right.result.wall_positions);
                    });

                const std::size_t countBefore{ temp.size() };

                temp.erase(
                    std::unique(
                        std::begin(temp),
                        std::end(temp),
                        [&](const Job & left, const Job & right) {
                            return (left.result.wall_positions == right.result.wall_positions);
                        }),
                    std::end(temp));

                const std::size_t countAfter{ temp.size() };
                std::cout << "\n\t WallPosition-Duplicates = " << (countBefore - countAfter);
            }

            std::cout << std::endl << std::endl;
        }

        // bool isCloseEnoughToEqual(
        //    const Spec& specLeft,
        //    const Result& resultLeft,
        //    const Spec& specRight,
        //    const Result& resultRight) const;

        // bool containsExact(const Spec& spec, const Result& result) const;
        // bool containsCloseEnoughToEqual(const Spec& spec, const Result& result) const;

        std::vector<Job> jobs;
    };

    // splits a given bounding rect into rows and columns
    struct Factory
    {
        static inline Warehouse warehouse;

        static bool make(
            Job & job,
            const sf::IntRect & bounds,
            const int count,
            const Feature_t features,
            const int border);

        static std::vector<Job> makeSeries(
            const sf::IntRect & bounds,
            const int count,
            const Feature_t features,
            const int border);

        // static IntRectVecVec_t makeIntRectsNested(const Job & job);

        // static IntRectVec_t flattenDoubleWidthWalls_Orig(
        //    const IntRectVec_t & rects,
        //    const Spec & spec,
        //    const sf::IntRect & bounds,
        //    ColCountPerRowVec_t & colCountPerRow);

        // static void flattenDoubleWidthWalls_ForSquareTiles(
        //    IntRectVecVec_t & rectsPerRow, const Spec & spec, const sf::IntRect & bounds);

        static void makeWallBoardPositionsFromTiles(Job & job);

        static void makeWallBoardPositionsFromTile(
            const sf::IntRect & rect,
            const bool willFill,
            BoardPosVec_t & wallPositions,
            const sf::IntRect & bounds = {});

        static void moveTilesTopLeftUntilWallsOverlap(Job & job);

        static void growTiles(Job & job);

        // returns the minimally enclosing bounding rect of ALL the tiles including those
        // added
        static void addTiles(Job & job);

        // static void reCenterTiles(Job & job);

        static std::size_t calcWallPositionsOfTile(const sf::IntRect & tile, const bool isFilled)
        {
            if (isFilled)
            {
                return static_cast<std::size_t>(tile.width * tile.height);
            }
            else
            {
                return static_cast<std::size_t>(((tile.width * 2) + (tile.height * 2)) - 4);
            }
        }

        static void setupRowsAndColsFromSpec(
            Result & result, const Spec & spec, const sf::IntRect & bounds);

        static void setupRowsAndColsFromRects(
            Result & result, const Spec & spec, const sf::IntRect & bounds);

        static void populateIntRects(Job & job);

        static std::size_t calcMaxCount(const Job & job)
        {
            if (job.spec.isRooms())
            {
                const sf::Vector2i oneOne(1, 1);
                const sf::Vector2i smallestRoom(3, 3);
                const sf::Vector2i smallestRoomMinusOne(smallestRoom - oneOne);

                const sf::Vector2i maxPossibleRoomCounts(
                    ((util::size(job.bounds) - smallestRoom) / smallestRoomMinusOne) + oneOne);

                const int maxPossibleRoomCount{ maxPossibleRoomCounts.x * maxPossibleRoomCounts.y };
                return maxPossibleRoomCount;
            }

            return 0; // means no max could be established
        }

        // static bool removeTilesUniform(IntRectVecVec_t & nestedRects, const int toRemoveCount);

      private:
        static bool adjustBounds(sf::IntRect & bounds, const int border)
        {
            // spec.border is the number of cells between tiles, but if that border > 0...
            // then this prevents those tiles from touching the sides of the walls
            if (border > 0)
            {
                bounds.left += 1;
                bounds.top += 1;
                bounds.width -= 2;
                bounds.height -= 2;
            }

            return ((bounds.width > 0) && (bounds.height > 0));
        }

        static bool isRequestedCountValid(const Job & job)
        {
            const std::size_t maxCount{ calcMaxCount(job) };
            return ((0 == maxCount) || (job.spec.count <= maxCount));
        }

        // If borderSize < 0 it's bex``zcomes the scale of the default border size (very
        // thin). (i.e. if borderSize == -10.0f, then the border will be ten times the
        // default.
        static std::vector<sf::FloatRect> makeFloatRects(const Job & job);

        static void populatedWithNestedVectorRects(
            const IntRectVec_t & flatRects,
            const Stats<int> & columns,
            IntRectVecVec_t & nestedRectResult);

        static IntRectOpt_t
            makeIntTileFromFloatTile(const sf::FloatRect & tileRect, const sf::IntRect & bounds);

        static ColCountPerRowVec_t makeRowsAndCols_uniform(const Spec & spec);

        // static ColCountPerRowVec_t makeRowsAndCols_irregular(const int count);

        static bool preCheckRowsAndColumns(
            const Result & result, const Spec & spec, const sf::IntRect & bounds);

        static bool preCheckRowsAndColumns(Job & job)
        {
            return preCheckRowsAndColumns(job.result, job.spec, job.bounds);
        }
    };
} // namespace snake::tile
*/
#endif // SNAKE_TILING_HPP_INCLUDED