// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// tiling.cpp
//
/*
#include "tiling.hpp"

#include "check-macros.hpp"
#include "settings.hpp"

#include <set>

namespace snake::tile::Feature
{
    std::string toName(const Feature_t feature)
    {
        // clang-format off
        switch(feature)
        {
            case None:            return "None?";
            case CountCanIncr:    return "Ct+++";
            case Filled:          return "Filld";
            case Obstacles:       return "Block";
            case OneRow:          return "1xRow";
            case OneColumn:       return "1xCol";
            case ColCountSame:    return "Cc=Rc";
            case ColRowCountSame: return "Cc=Cc";
            case RectsSameArea:   return "R=Are";
            case RectsSameShape:  return "R=Shp";
            case RectsSquare:     return "R=Squ";
            default:              return "";
        }
        // clang-format on
    }

    std::vector<std::string> toStrings(const Feature_t features, const bool willSkipDefaults)
    {
        std::vector<std::string> vec;

        // clang-format off
        if (features & Filled   )    { vec.push_back("Filld"); } else if (!willSkipDefaults) {
vec.push_back("Hollo"); } if (features & Obstacles)    { vec.push_back("Block"); } else if
(!willSkipDefaults) { vec.push_back("Rooms"); } if (features & CountCanIncr) {
vec.push_back("Ct+++"); } else if (!willSkipDefaults) { vec.push_back("Exact"); }

        if      (features & ColRowCountSame)    { vec.push_back("Cc=Rc"); }
        else if (features & ColCountSame)       { vec.push_back("Cc=Cc"); }

        if      (features & OneRow)             { vec.push_back("1xRow"); }
        else if (features & OneColumn)          { vec.push_back("1xCol"); }

        if      (features & RectsSquare)        { vec.push_back("R=Squ"); }
        else if (features & RectsSameShape)     { vec.push_back("R=Shp"); }
        else if (features & RectsSameArea)      { vec.push_back("R=Are"); }
        // clang-format on

        return vec;
    }

    void populateDiffStrings(
        const Feature_t from,
        const Feature_t to,
        std::vector<std::string> & losts,
        std::vector<std::string> & gains)
    {
        std::string tempStr;

        for (std::size_t i(0); i <= 9; ++i)
        {
            const Feature_t featureToCheck{ static_cast<Feature_t>(1 << i) };

            if ((featureToCheck == Feature::Filled) || (featureToCheck == Feature::Obstacles) ||
                (featureToCheck == Feature::None))
            {
                continue;
            }

            const Feature_t isSetInFrom{ (from & featureToCheck) != 0 };
            const Feature_t isSetInTo{ (to & featureToCheck) != 0 };

            if (isSetInFrom == isSetInTo)
            {
                continue;
            }

            if ((featureToCheck & Feature::ColCountSame) && (to & Feature::ColRowCountSame))
            {
                continue;
            }

            if (featureToCheck & (Feature::RectsSameArea | Feature::RectsSameShape))
            {
                if (to & (Feature::RectsSameShape | Feature::RectsSquare))
                {
                    continue;
                }
            }

            tempStr = ((isSetInFrom) ? "-" : "+");
            tempStr += toName(featureToCheck);

            if (isSetInFrom)
            {
                losts.emplace_back(tempStr);
            }
            else
            {
                gains.emplace_back(tempStr);
            }
        }
    }
} // namespace snake::tile::Feature

//

namespace snake::tile
{
    std::string Spec::toString() const
    {
        std::ostringstream ss;

        // ss << "Spec: ";

        if (isBlocks())
        {
            ss << "Blocks, ";
        }
        else if (isRooms())
        {
            ss << "Rooms, ";
        }
        else if (count > 0)
        {
            ss << "TILE_ERROR:(intent is neither Rooms or Blocks!), ";
        }

        if (isOneDimm() && (count > 1))
        {
            ss << "1D, ";
        }

        if (border != 0)
        {
            ss << "border=" << border << ", ";
        }

        ss << Feature::toString(features, false);

        return ss.str();
    }

    //

    void Result::reset()
    {
        is_success = false;

        is_valid_rooms = false;
        is_valid_bocks = false;
        is_valid_one_dimm = false;

        features = Feature::None;

        shapes.reset();
        areas.reset();
        hollow_areas.reset();
        columns.reset();

        rects.clear(); // reserve is called where the real count is known

        wall_positions.clear();
        wall_positions.reserve(4096); // a typical 60x50 cell board only needs 3000

        diff_str.clear();
        summary_str.clear();
    }

    void Result::setup(Job & job)
    {
        // Result.is_success is already == false so simply "return" on any error

        job.result.reset();

        Factory::setupRowsAndColsFromSpec(job.result, job.spec, job.bounds);
        if (!job.result.columns.isValid())
        {
            return;
        }

        Factory::populateIntRects(job);
        if (job.result.rects.empty() || job.result.rects.front().empty() ||
            (job.result.tileCount() < job.spec.count) ||
            (job.result.rects.size() != job.result.rowCount()))
        {
            return;
        }

        sort();

        // If you want independant rectangles with walls that do not touch, then don't run this.
        // This is for when you just want single thin lines drawn end-to-end to subdivide, in
        // whcih case none of the walls shuld be side-by-side because that would just waste
        // space.  This function combines such walls into one single line.
        // if (job.spec.isRooms())
        {
            // std::cout << "\nTile Temp Info: " << tile::Feature::toString(job.spec.features);
            // std::cout << ", spec_count=" << job.spec.count;
            // std::cout << "\norig:  cols=" << job.result.columns.list_str;
            // std::cout << ", bounds=" << job.bounds;
            // std::cout << ", total_size=" << origSize;
            // std::cout << ", rows/cols=" << job.result.rowCount() << 'x' <<
            // job.result.columns.max;

            Factory::moveTilesTopLeftUntilWallsOverlap(job);
            // const sf::IntRect minEnclosingRectAfterMove{ job.result.minEnclosingRect() };

            // std::cout << "\nmoved: spec_count=" << job.spec.count
            //          << ", cols=" << job.result.columns.list_str;
            // std::cout << ", total_size=" << util::size(minEnclosingRectAfterMove);
            // std::cout << ", rows/cols=" << job.result.rowCount() << 'x' <<
            // job.result.columns.max;
            //
            // std::cout << ", tile_sizes=" << util::size(rects.front().front()) << " / "
            //          << util::size(rects.back().back());
            //
            // M_CHECK(util::position(job.bounds) == util::position(minEnclosingRectAfterMove));

            Factory::growTiles(job);

            // std::cout << "\nGrown:  spec_count=" << job.spec.count
            //          << ", cols=" << job.result.columns.list_str;
            // std::cout << ", total_size=" << util::size(minEnclosingRectAfterGrow);
            // std::cout << ", rows/cols=" << job.result.rowCount() << 'x' <<
            // job.result.columns.max;
            //
            // std::cout << ", tile_sizes=" << util::size(rects.front().front()) << " / "
            //          << util::size(rects.back().back());
            //
            // M_CHECK(util::position(job.bounds) == util::position(minEnclosingRectAfterGrow));

            if (job.spec.features & Feature::CountCanIncr)
            {
                Factory::addTiles(job);

                // std::cout << "\nadded:  spec_count=" << job.spec.count
                //          << ", cols=" << job.result.columns.list_str;
                // std::cout << ", total_size=" << util::size(minEnclosingRectAfterAdd);
                //
                // std::cout << ", rows/cols=" << job.result.rowCount() << 'x'
                //          << job.result.columns.max;
                //
                // std::cout << ", tile_sizes=" << util::size(rects.front().front()) << " / "
                //          << util::size(rects.back().back());
                //
                // M_CHECK(util::position(job.bounds) == util::position(minEnclosingRectAfterAdd));
            }

            // Factory::reCenterTiles(job);

            // std::cout << std::endl;
        }

        // TODO center per row

        Factory::makeWallBoardPositionsFromTiles(job);
        if (job.result.wall_positions.empty())
        {
            return;
        }

        if (!populateAndVerifyStats(job.spec))
        {
            return;
        }

        detectFeatures(job.spec);
        verifyFeatures(job.spec);

        findMatchingStyles();

        static std::vector<std::string> losts;
        losts.clear();

        static std::vector<std::string> gains;
        gains.clear();

        is_success = determineSuccess(job.spec, losts, gains);

        // TODO detect repeats

        diff_str = makeDiffString(losts, gains);
        summary_str = makeSummaryString(job.spec, job.bounds);
    }

    void Factory::addTiles(Job & job)
    {
        if (!(job.spec.features & Feature::CountCanIncr))
        {
            return;
        }

        IntRectVecVec_t & rects{ job.result.rects };
        const auto rectsOrigCopy{ rects };

        if (!job.result.columns.isValid() || rects.empty() || rects.front().empty() ||
            rects.front().front().width < 2)
        {
            return;
        }

        const sf::IntRect minEnclosingRectBefore{ job.result.minEnclosingRect() };

        const sf::Vector2i freeBoardSpaceBefore(
            util::size(job.bounds) - job.result.minEnclosingSize());

        const sf::IntRect rectCopy(rects.front().front());
        const sf::Vector2i rectCopySize(util::size(rectCopy));
        const sf::Vector2i rectCopySizeMinusOne(rectCopySize - sf::Vector2i(1, 1));
        const sf::Vector2i tilesToAddCount{ freeBoardSpaceBefore / rectCopySizeMinusOne };

        if ((tilesToAddCount.x <= 0) && (tilesToAddCount.y <= 0))
        {
            return;
        }

        // sf::Vector2i bottomRightPosAfter(
        //    util::right(rects.back().back()), util::bottom(rects.back().back()));

        if (tilesToAddCount.x > 0)
        {
            for (IntRectVec_t & row : rects)
            {
                for (int i(0); i < tilesToAddCount.x; ++i)
                {
                    sf::IntRect & newRect{ row.emplace_back(row.back()) };

                    newRect.left += rectCopySizeMinusOne.x;
                }
            }
        }

        if (tilesToAddCount.y > 0)
        {
            for (int i(0); i < tilesToAddCount.y; ++i)
            {
                IntRectVec_t bottomRow{ rects.back() };

                for (sf::IntRect & rect : bottomRow)
                {
                    rect.top += rectCopySizeMinusOne.y;
                }

                rects.push_back(bottomRow);
            }
        }

        // TODO Remove?  this is probably NOT needed
        job.result.sort();

        // re-setup the columns into
        setupRowsAndColsFromRects(job.result, job.spec, job.bounds);
    }

    void Factory::growTiles(Job & job)
    {
        const sf::Vector2i freeBoardSpaceBefore(
            util::size(job.bounds) - job.result.minEnclosingSize());

        const bool isEnoughBoardFreeSpaceToAttemptGrowth{
            (freeBoardSpaceBefore.x >= job.result.columns.max) ||
            (freeBoardSpaceBefore.y >= job.result.rowCount())
        };

        if (!isEnoughBoardFreeSpaceToAttemptGrowth)
        {
            return;
        }

        sf::Vector2i bottomRightPosAfter(0, 0);
        const sf::Vector2i BottomRightBoard(util::right(job.bounds), util::bottom(job.bounds));

        auto attemptToGrow = [BottomRightBoard](
                                 IntRectVecVec_t & rectsCopy,
                                 sf::Vector2i & bottomRightPosAfterCopy) {
            for (std::size_t rowIndex(0); rowIndex < rectsCopy.size(); ++rowIndex)
            {
                IntRectVec_t & row{ rectsCopy.at(rowIndex) };

                for (std::size_t colIndex(0); colIndex < row.size(); ++colIndex)
                {
                    sf::IntRect & rect{ row.at(colIndex) };

                    rect.width += 1;
                    rect.height += 1;
                    rect.left += static_cast<int>(colIndex);
                    rect.top += static_cast<int>(rowIndex);

                    bottomRightPosAfterCopy.x =
                        std::max(bottomRightPosAfterCopy.x, util::right(rect));

                    bottomRightPosAfterCopy.y =
                        std::max(bottomRightPosAfterCopy.y, util::bottom(rect));

                    if ((bottomRightPosAfterCopy.x > BottomRightBoard.x) ||
                        (bottomRightPosAfterCopy.y > BottomRightBoard.y))
                    {
                        return false;
                    }
                }
            }

            return true;
        };

        std::size_t growCount{ 0 };
        IntRectVecVec_t rectsCopy{ job.result.rects };
        sf::Vector2i bottomRightPosAfterCopy{ bottomRightPosAfter };
        while (attemptToGrow(rectsCopy, bottomRightPosAfterCopy))
        {
            ++growCount;
            job.result.rects = rectsCopy;
            bottomRightPosAfter = bottomRightPosAfterCopy;
        }
    }

    bool Result::determineSuccess(
        const Spec & spec, std::vector<std::string> & losts, std::vector<std::string> & gains)
    {
        if (empty())
        {
            return false;
        }

        if (!shapes.isValid() || !areas.isValid() || !hollow_areas.isValid() || !columns.isValid())
        {
            return false;
        }

        bool success{ true };

        // counts
        const int tile_count_exp{ spec.count };
        const int tile_count_act{ tileCount() };
        M_CHECK(tile_count_act >= tile_count_exp);

        if (tile_count_act > tile_count_exp)
        {
            std::string tempStr{ "+" + std::to_string(tile_count_act - tile_count_exp) };
            const std::size_t targetWidth{ 6 };
            if (tempStr.length() < targetWidth)
            {
                tempStr.insert(0, (targetWidth - tempStr.length()), ' ');
            }

            gains.push_back(tempStr);
        }

        if (!(spec.features & Feature::CountCanIncr) && (tile_count_exp != tile_count_act))
        {
            success = false;
        }

        // intents
        if (spec.isBlocks() != is_valid_bocks)
        {
            if (is_valid_bocks)
            {
                gains.push_back("+Block");
            }
            else
            {
                losts.push_back("-Block");
            }

            if (spec.isBlocks())
            {
                success = false;
            }
        }

        if (spec.isRooms() != is_valid_rooms)
        {
            if (is_valid_rooms)
            {
                gains.push_back("+Rooms");
            }
            else
            {
                losts.push_back("-Rooms");
            }

            if (spec.isRooms())
            {
                success = false;
            }
        }

        if (spec.isOneDimm() != is_valid_one_dimm)
        {
            const std::string tempStr{ (spec.features & Feature::OneRow) ? "1xRow" : "1xCol" };

            if (is_valid_one_dimm)
            {
                gains.emplace_back("+" + tempStr);
            }
            else
            {
                losts.emplace_back("-" + tempStr);
            }

            if (spec.isOneDimm())
            {
                success = false;
            }
        }

        const std::size_t lostsCountBefore{ losts.size() };
        Feature::populateDiffStrings(spec.features, features, losts, gains);
        const std::size_t lostsCountAfter{ losts.size() };
        if (lostsCountBefore < lostsCountAfter)
        {
            success = false;
        }

        const std::size_t totalRectCount = std::reduce(
            std::begin(rects),
            std::end(rects),
            0_st,
            [](const std::size_t sumSoFar, const IntRectVec_t & row) {
                return (sumSoFar + row.size());
            });

        if ((tileCount() != columns.sum) || (tileCount() != static_cast<int>(totalRectCount)) ||
            (tileCount() != shapes.count()) || (tileCount() != areas.count()) ||
            (tileCount() != hollow_areas.count()))
        {
            success = false;
        }

        return success;
    }

    std::string Result::makeDiffString(
        std::vector<std::string> & losts, std::vector<std::string> & gains) const
    {
        std::ostringstream ss;

        // ss << "Diff:";
        ss << '(';

        const std::string lostsStr{ util::containerToString(losts, ", ") };
        const std::string gainsStr{ util::containerToString(gains, ", ") };

        ss << lostsStr;

        if (!lostsStr.empty() && !gainsStr.empty())
        {
            ss << " / ";
        }

        ss << gainsStr;
        ss << ')';

        return ss.str();
    }

    std::string Result::makeSummaryString(const Spec & spec, const sf::IntRect & bounds) const
    {
        std::ostringstream ss;

        ss << "Result: " << spec.toString();
        ss << ", cells=" << util::size(bounds);
        ss << ", count=" << std::setw(3) << std::right << spec.count;
        ss << '/' << std::setw(3) << std::left << tileCount();
        ss << ", " << ((is_success) ? "PASS" : "FAIL");
        ss << ",  " << std::setw(40) << std::left << diff_str;

        ss << ", Cols";
        if (columns.unique_count > 1)
        {
            ss << columns.unique_count << "u";
        }
        ss << '(';

        ss << std::setw(8) << std::left << (columns.list_str + ")");

        ss << ", Shapes";
        if (shapes.unique_count > 1)
        {
            ss << shapes.unique_count << "u";
        }
        ss << '(';

        ss << std::setw(15) << std::left << (shapes.list_str + ")");

        return ss.str();
    }

    bool Result::populateAndVerifyStats(const Spec & spec)
    {
        const bool isFilled{ (spec.features & Feature::Filled) != 0 };

        for (const IntRectVec_t & row : rects)
        {
            for (const sf::IntRect & rect : row)
            {
                shapes.values.emplace_back(rect.width, rect.height);
                areas.values.emplace_back(rect.width * rect.height);

                const bool isSizeValid{ (rect.width > 2) && (rect.height > 2) };
                const int hollowArea{ (rect.width - 2) * (rect.height - 2) };

                if (isFilled || !isSizeValid || (hollowArea <= 0))
                {
                    hollow_areas.values.push_back(0);
                }
                else
                {
                    hollow_areas.values.push_back(hollowArea);
                }
            }
        }

        shapes.setup();
        areas.setup();
        hollow_areas.setup();
        // columns.setup(); //intentionally skipped, see comment below
        //
        // The Result::columns Stat member variable is the only Stat that is not populated and
        // setup() here, because it changes as the rows/cols are figured out in the Result::setup()
        // function above.  So leave columns alone.  It has already been populated and has had it's
        // setup() function called.

        if (!columns.isValid() || !shapes.isValid() || !areas.isValid() || !hollow_areas.isValid())
        {
            return false;
        }

        if ((rowCount() > tileCount()) || (columns.max > tileCount()))
        {
            return false;
        }

        if ((shapes.count() != tileCount()) || (areas.count() != tileCount()))
        {
            return false;
        }

        if (hollow_areas.count() != areas.count())
        {
            return false;
        }

        if (hollow_areas.sum > areas.sum)
        {
            return false;
        }

        if (columns.count() != rowCount())
        {
            return false;
        }

        if ((columns.min == columns.max) != (columns.unique_count == 1))
        {
            return false;
        }

        return true;
    }

    void Result::detectFeatures(const Spec & spec)
    {
        M_CHECK(spec.count <= tileCount());

        if (spec.features & Feature::CountCanIncr)
        {
            features |= Feature::CountCanIncr;
        }
        else
        {
            if (spec.count == tileCount())
            {
                features |= Feature::CountCanIncr;
            }
        }

        if (spec.features & Feature::Filled)
        {
            features |= Feature::Filled;
        }

        if (spec.features & Feature::Obstacles)
        {
            features |= Feature::Obstacles;
        }

        if (rowCount() == 1)
        {
            features |= Feature::OneRow;
        }

        if (columns.max == 1)
        {
            features |= Feature::OneColumn;
        }

        if (columns.min == columns.max)
        {
            features |= Feature::ColCountSame;

            if (columns.max == rowCount())
            {
                features |= Feature::ColRowCountSame;
            }
        }

        if (tileCount() == 1)
        {
            features |= Feature::OneColumn;
            features |= Feature::OneRow;
        }

        if (areas.unique_count == 1)
        {
            features |= Feature::RectsSameArea;
        }

        if (shapes.unique_count == 1)
        {
            features |= Feature::RectsSameShape;
        }

        bool areAllSquare{ true };
        for (const sf::Vector2i & size : shapes.values)
        {
            if (size.x != size.y)
            {
                areAllSquare = false;
                break;
            }
        }

        if (areAllSquare)
        {
            features |= Feature::RectsSquare;
        }
    }

    void Result::verifyFeatures(const Spec & spec) const
    {
        M_CHECK(bool(features & Feature::OneRow) == (1 == rowCount()));
        M_CHECK(bool(features & Feature::OneColumn) == (columns.max == 1));

        if (spec.features & Feature::CountCanIncr)
        {
            M_CHECK(spec.count <= tileCount());
        }
        else
        {
            M_CHECK(columns.sum == tileCount());
        }

        if (features & Feature::RectsSquare)
        {
            M_CHECK(features & Feature::RectsSameShape);
            M_CHECK(features & Feature::RectsSameArea);
        }

        if (features & Feature::RectsSameShape)
        {
            M_CHECK(features & Feature::RectsSameArea);
        }

        if (features & Feature::ColRowCountSame)
        {
            M_CHECK(features & Feature::ColCountSame);
            M_CHECK(columns.min == columns.max);
            M_CHECK(columns.unique_count == 1);
        }

        if (features & Feature::ColCountSame)
        {
            M_CHECK(columns.min == columns.max);
            M_CHECK(columns.unique_count == 1);
        }
    }

    void Result::findMatchingStyles()
    {
        is_valid_rooms = false;
        is_valid_bocks = false;
        is_valid_one_dimm = false;

        if (empty())
        {
            return;
        }

        //
        is_valid_rooms =
            (!hollow_areas.empty() && (hollow_areas.min > 0) && !(features & Feature::Obstacles));

        // is_valid_bocks
        //{
        //    is_valid_bocks = true;
        //
        //   // const sf::Vector2i oneOne(1, 1);
        //
        //    // loop over all tile rects and make sure none of them overlap
        //    for (std::size_t outer(0); outer < rects.size(); ++outer)
        //    {
        //        const sf::IntRect & outerRect(rects[outer]);
        //
        //        const sf::IntRect rectToCheck(
        //            (outerRect.left - 1),
        //            (outerRect.top - 1),
        //            (outerRect.width + 2),
        //            (outerRect.height + 2));
        //
        //        if ((rectToCheck.left < 1) || (rectToCheck.top < 1) || (rectToCheck.width < 1)
        //        ||
        //            (rectToCheck.height < 1))
        //        {
        //            is_valid_bocks = false;
        //            break;
        //        }
        //
        //        for (std::size_t inner(outer + 1); inner < rects.size(); ++inner)
        //        {
        //            const sf::IntRect & innerRect(rects[inner]);
        //
        //            if (rectToCheck.intersects(innerRect))
        //            {
        //                is_valid_bocks = false;
        //                break;
        //            }
        //        }
        //
        //        if (!is_valid_bocks)
        //        {
        //            break;
        //        }
        //    }
        //}

        // is_valid_one_dimm
        {
            const bool isOneValidColByFeatures{ (features & Feature::OneColumn) &&
                                                (tileCount() == columns.count()) &&
                                                (columns.min == columns.max) &&
                                                (1 == columns.min) };

            const bool isOneValidRowByFeatures{ (features & Feature::OneRow) &&
                                                (columns.count() == 1) };

            const auto & [isOneValidColByPosition, isOneValidRowByPosition] = [&]() {
                std::pair<bool, bool> areAllLeftRightEqualPair(true, true);
                const sf::IntRect firstTile{ rects.front().front() };

                for (const IntRectVec_t & rowOfTiles : rects)
                {
                    for (const sf::IntRect & tile : rowOfTiles)
                    {
                        if (tile.left != firstTile.left)
                        {
                            areAllLeftRightEqualPair.first = false;
                        }

                        if (tile.top != firstTile.top)
                        {
                            areAllLeftRightEqualPair.second = false;
                        }

                        if (!areAllLeftRightEqualPair.first && !areAllLeftRightEqualPair.second)
                        {
                            break;
                        }
                    }
                }

                return areAllLeftRightEqualPair;
            }();

            M_CHECK(isOneValidColByFeatures == isOneValidColByPosition);
            M_CHECK(isOneValidRowByFeatures == isOneValidRowByPosition);

            is_valid_one_dimm =
                ((isOneValidColByFeatures && isOneValidColByPosition) ||
                 (isOneValidRowByFeatures == isOneValidRowByPosition));
        }
    }

    //

    bool Factory::preCheckRowsAndColumns(
        const Result & result, const Spec & spec, const sf::IntRect & bounds)
    {
        if (!result.columns.isValid())
        {
            return false;
        }

        const int specTileCount{ spec.count };
        const int colTileCount{ result.columns.sum };
        if ((colTileCount != result.tileCount()) || (specTileCount > colTileCount))
        {
            return false;
        }

        const int rowCount{ result.columns.count() };
        if (rowCount != result.rowCount())
        {
            return false;
        }

        if ((result.columns.max > bounds.width) || (rowCount > bounds.height))
        {
            return false;
        }

        if ((spec.features & Feature::OneRow) && (rowCount != 1))
        {
            return false;
        }

        if ((spec.features & Feature::OneColumn) && (result.columns.max != 1))
        {
            return false;
        }

        if ((spec.features & Feature::ColCountSame) && (result.columns.min != result.columns.max))
        {
            return false;
        }

        if ((spec.features & Feature::ColRowCountSame) &&
            ((result.columns.min != result.columns.max) || (result.columns.unique_count != 1)))
        {
            return false;
        }

        // intentionally skip this check so that it can be worked around later in useful ways
        // if ((!(spec.features & Feature::CountCanIncr)) && (specTileCount != colTileCount))
        //{
        //    return false;
        //}

        return true;
    }

    bool Factory::make(
        Job & job,
        const sf::IntRect & bounds,
        const int count,
        const Feature_t features,
        const int border)
    {
        job.bounds = bounds;
        if (!adjustBounds(job.bounds, border))
        {
            return false;
        }

        job.spec = Spec(count, features, border);
        if (!job.spec.isValid())
        {
            return false;
        }

        if (!isRequestedCountValid(job))
        {
            return false;
        }

        job.result.setup(job);
        std::cout << job.toString() << '\n';
        if (!job.result.isValid() || !job.isValid())
        {
            return false;
        }

        Factory::warehouse.jobs.push_back(job);
        return true;
    }

    std::vector<Job> Factory::makeSeries(
        const sf::IntRect & bounds, const int count, const Feature_t features, const int border)
    {
        std::vector<Job> jobs;
        Job & job{ jobs.emplace_back() };

        job.bounds = bounds;
        if (!adjustBounds(job.bounds, border))
        {
            return {};
        }

        job.spec = Spec(count, features, border);
        if (!job.spec.isValid())
        {
            return {};
        }

        if (!isRequestedCountValid(job))
        {
            return {};
        }

        job.result.reset();

        return jobs;
    }

    void Factory::makeWallBoardPositionsFromTiles(Job & job)
    {
        const bool willFillTiles{ (job.spec.features & Feature::Filled) != 0 };

        const std::size_t wallPositionTotalCountRequired = std::reduce(
            std::begin(job.result.rects),
            std::end(job.result.rects),
            0_st,
            [&](const std::size_t sumSoFar, const IntRectVec_t & row) {
                std::size_t count{ sumSoFar };
                for (const sf::IntRect & rect : row)
                {
                    count += calcWallPositionsOfTile(rect, willFillTiles);
                }

                return count;
            });

        M_CHECK(static_cast<int>(wallPositionTotalCountRequired) >= job.spec.count);
        M_CHECK(static_cast<int>(wallPositionTotalCountRequired) >= job.result.tileCount());

        // const std::size_t wallPositionTotalCountRequired = [&]() {
        //    std::size_t count{ 0 };
        //    for (const IntRectVec_t & row : job.result.rects)
        //    {
        //        for (const sf::IntRect & rect : row)
        //        {
        //            count += calcWallPositionsOfTile(rect, willFillTiles);
        //        }
        //    }
        //
        //    return count;
        //}();

        job.result.wall_positions.reserve(
            job.result.wall_positions.size() + wallPositionTotalCountRequired);

        for (const IntRectVec_t & row : job.result.rects)
        {
            for (const sf::IntRect & rect : row)
            {
                makeWallBoardPositionsFromTile(rect, willFillTiles, job.result.wall_positions);
            }
        }

        // some duplicates are expected and not errors up until now,
        // but this is where they need to be removed and considered errors after.
        // In fact nothing should change result.wall_positions after this.
        auto & posVec{ job.result.wall_positions };
        std::sort(std::begin(posVec), std::end(posVec));

        job.result.wall_positions.erase(
            std::unique(std::begin(posVec), std::end(posVec)), std::end(posVec));

        M_CHECK(static_cast<int>(posVec.size()) >= job.spec.count);
        M_CHECK(static_cast<int>(posVec.size()) >= job.result.tileCount());
    }

    void Factory::makeWallBoardPositionsFromTile(
        const sf::IntRect & rect,
        const bool willFill,
        BoardPosVec_t & wallPositions,
        const sf::IntRect & boundsOrig)
    {
        const sf::IntRect & bounds{ ((boundsOrig.width > 0) && (boundsOrig.height > 0)) ? boundsOrig
                                                                                        : rect };

        if ((rect.width <= 0) || (rect.height <= 0) || (bounds.width <= 0) || (bounds.height <= 0))
        {
            return;
        }

        // shrink the tile's rect to avoid creating any invalid positions
        const int left{ std::max(rect.left, bounds.left) };
        const int right{ std::min(util::right(rect), util::right(bounds)) };

        const int top{ std::max(rect.top, bounds.top) };
        const int bottom{ std::min(util::bottom(rect), util::bottom(bounds)) };

        // shirnk the vertical bounds inward to avoid appending duplicate positions
        const int topPlusOne{ top + 1 };
        const int bottomMinusOne{ bottom - 1 };

        // if the rect is filled, then appends exactly (rect.width * rect.height) positions
        if (willFill)
        {
            for (int horiz(left); horiz < right; ++horiz)
            {
                for (int vert(topPlusOne); vert < bottomMinusOne; ++vert)
                {
                    wallPositions.emplace_back(horiz, vert);
                }
            }
        }
        else
        {
            // if hollow, then appends exactly ((rect.width * 2) + (rect.height * 2)) - 4)
            const int rightMinusOne{ right - 1 };
            const bool isWidthTwoOrMore{ rect.width >= 2 };
            const bool isHeightTwoOrMore{ rect.height >= 2 };

            for (int horiz(left); horiz < right; ++horiz)
            {
                wallPositions.emplace_back(horiz, top);

                if (isHeightTwoOrMore)
                {
                    wallPositions.emplace_back(horiz, bottomMinusOne);
                }
            }

            for (int vert(topPlusOne); vert < bottomMinusOne; ++vert)
            {
                wallPositions.emplace_back(left, vert);

                if (isWidthTwoOrMore)
                {
                    wallPositions.emplace_back(rightMinusOne, vert);
                }
            }
        }
    }

    std::vector<sf::FloatRect> Factory::makeFloatRects(const Job & job)
    {
        using namespace util;

        const sf::FloatRect boundsF(job.bounds);
        const ColCountPerRowVec_t & colCountPerRow{ job.result.columns.values };

        if (!job.result.columns.isValid() || (boundsF.width < 1.0f) || (boundsF.height < 1.0f))
        {
            return {};
        }

        const float borderSize = [&]() {
            if (job.spec.border < 0)
            {
                const float defaultBorder{ std::pow((boundsF.width + boundsF.height), 0.25f) };
                return (defaultBorder * static_cast<float>(-job.spec.border));
            }
            else
            {
                return static_cast<float>(job.spec.border);
            }
        }();

        const std::size_t rowCount{ static_cast<std::size_t>(colCountPerRow.size()) };
        const float borderHeightSum(static_cast<float>(rowCount + 1) * borderSize);
        const float rowHeight{ (boundsF.height - borderHeightSum) / static_cast<float>(rowCount) };
        if (rowHeight < 1.0f)
        {
            return {};
        }

        std::vector<sf::FloatRect> rects;
        for (std::size_t rowIndex(0); rowIndex < rowCount; ++rowIndex)
        {
            const std::size_t colCount{ static_cast<std::size_t>(colCountPerRow[rowIndex]) };

            if (0 == colCount)
            {
                continue;
            }

            const float borderWidthSum(static_cast<float>(colCount + 1) * borderSize);
            const float colWidth{ (boundsF.width - borderWidthSum) / static_cast<float>(colCount) };

            const float top{ boundsF.top + (static_cast<float>(rowIndex + 1) * borderSize) +
                             (static_cast<float>(rowIndex) * rowHeight) };

            for (std::size_t colIndex(0); colIndex < colCount; ++colIndex)
            {
                const float left{ boundsF.left + (static_cast<float>(colIndex + 1) * borderSize) +
                                  (static_cast<float>(colIndex) * colWidth) };

                const sf::FloatRect rect({ left, top }, { colWidth, rowHeight });

                if (job.spec.features & Feature::RectsSquare)
                {
                    const sf::Vector2f rectCenter{ center(rect) };

                    const float squareSideLength{ std::min(colWidth, rowHeight) };
                    const sf::Vector2f squareSize(squareSideLength, squareSideLength);

                    const sf::FloatRect squareRect{ (rectCenter - (squareSize * 0.5f)),
                                                    squareSize };
                    rects.push_back(squareRect);
                }
                else
                {
                    rects.push_back(rect);
                }
            }
        }

        for (sf::FloatRect & rect : rects)
        {
            auto isDiffLessThanOne = [&](const float left, const float right) {
                return (std::abs(left - right) < 1.0f);
            };

            auto fixMinorBoundsViolations = [&](sf::FloatRect & rct, const sf::FloatRect & bnd) {
                if ((rct.left < bnd.left) && isDiffLessThanOne(rct.left, bnd.left))
                {
                    rct.left = bnd.left;
                }

                if ((rct.top < bnd.top) && isDiffLessThanOne(rct.top, bnd.top))
                {
                    rct.top = bnd.top;
                }

                if ((right(rct) > right(bnd)) && isDiffLessThanOne(right(rct), right(bnd)))
                {
                    rct.width -= (right(rct) - right(bnd));
                }

                if ((bottom(rct) > bottom(bnd)) && isDiffLessThanOne(bottom(rct), bottom(bnd)))
                {
                    rct.height -= (bottom(rct) - bottom(bnd));
                }
            };

            fixMinorBoundsViolations(rect, boundsF);

            // check for values that are outside the given boundsF
            if ((rect.left < boundsF.left) || (rect.top < boundsF.top) ||
                (right(rect) > right(boundsF)) || (bottom(rect) > bottom(boundsF)))
            {
                M_LOG_SS(
                    "ERROR:  One of the generated rects " << rect << " went outside the boundsF "
                                                          << boundsF);

                return {};
            }

            if ((rect.width < 1.0f) && (std::abs(rect.width - 1.0f) < 0.0001f))
            {
                rect.width = 1.0f;
            }

            if ((rect.height < 1.0f) && (std::abs(rect.height - 1.0f) < 0.0001f))
            {
                rect.height = 1.0f;
            }

            // check for invalid sizes
            if ((rect.width < 1.0f) || (rect.height < 1.0f))
            {
                M_LOG_SS("ERROR:  One of the generated rects " << rect << " has an invalid size");
                return {};
            }
        }

        return rects;
    }

    void Factory::populateIntRects(Job & job)
    {
        IntRectVec_t rectsI;
        rectsI.reserve(64);

        const std::vector<sf::FloatRect> rectsF{ makeFloatRects(job) };
        for (const sf::FloatRect & rectF : rectsF)
        {
            const IntRectOpt_t rectIOpt{ makeIntTileFromFloatTile(rectF, job.bounds) };
            if (!rectIOpt)
            {
                M_LOG_SS(
                    "(" << job.spec.toString() << "\n  {" << job.result.columns.toString()
                        << "}\n  bounds" << job.bounds
                        << "}\n TILE_ERROR:  makeIntTileFromFloatTile(rectf=" << rectF
                        << ") (which was #" << rectsI.size() << " out of " << rectsF.size()
                        << ") was unable to turn that FloatRect into an IntRect with a valid "
                           "position and size.");

                return;
            }

            rectsI.push_back(rectIOpt.value());
        }

        populatedWithNestedVectorRects(rectsI, job.result.columns, job.result.rects);
    }

    void Factory::setupRowsAndColsFromSpec(
        Result & result, const Spec & spec, const sf::IntRect & bounds)
    {
        if ((spec.count <= 0) || (spec.count > (bounds.width * bounds.height)))
        {
            return;
        }

        result.columns.reset();

        if (spec.count == 1)
        {
            result.columns.values = { 1 };
        }
        else if (spec.count == 2)
        {
            if (bounds.width < bounds.height)
            {
                result.columns.values = { 1, 1 };
            }
            else
            {
                result.columns.values = { 2 };
            }
        }
        else
        {
            result.columns.values = makeRowsAndCols_uniform(spec);
        }

        result.columns.setup();

        if (!preCheckRowsAndColumns(result, spec, bounds))
        {
            result.columns.reset();
        }
    }

    void Factory::setupRowsAndColsFromRects(
        Result & result, const Spec & spec, const sf::IntRect & bounds)
    {
        result.columns.reset();

        for (const IntRectVec_t & row : result.rects)
        {
            result.columns.values.push_back(static_cast<int>(row.size()));
        }

        result.columns.setup();

        const bool isPreCheckValid{ preCheckRowsAndColumns(result, spec, bounds) };

        const bool isCountValid{ (spec.count == result.columns.sum) ||
                                 (spec.features & Feature::CountCanIncr) };

        if (!isPreCheckValid || !isCountValid || !result.columns.isValid())
        {
            result.columns.reset();
        }
    }

    void Factory::populatedWithNestedVectorRects(
        const IntRectVec_t & flatRects,
        const Stats<int> & columns,
        IntRectVecVec_t & nestedRectResult)
    {
        nestedRectResult.clear();
        nestedRectResult.reserve(columns.values.size());

        auto flatRectsIter{ std::begin(flatRects) };
        for (const int colCount : columns.values)
        {
            const auto rowBeginIter{ flatRectsIter };
            flatRectsIter += colCount;
            const auto rowEnd{ flatRectsIter };
            nestedRectResult.emplace_back(rowBeginIter, rowEnd);
        }
    }

    // IntRectVecVec_t
    //    Factory::makeFlatRectVecNested(const Stats<int>& columns, const IntRectVec_t& rectsFlat)
    //{
    //    IntRectVecVec_t rectsPerRow;
    //    rectsPerRow.reserve(256);
    //
    //    std::size_t rectsIndex{ 0 };
    //    for (const int colCountInt : columns.values)
    //    {
    //        M_CHECK(colCountInt > 0);
    //
    //        const std::size_t colCountST{ static_cast<std::size_t>(colCountInt) };
    //
    //        IntRectVec_t rectsInThisRow;
    //        rectsInThisRow.reserve(colCountST);
    //
    //        for (int i(0); i < colCountInt; ++i)
    //        {
    //            M_CHECK(rectsIndex < rectsFlat.size());
    //            rectsInThisRow.push_back(rectsFlat.at(rectsIndex++));
    //        }
    //
    //        M_CHECK(rectsInThisRow.size() == colCountST);
    //        rectsPerRow.push_back(rectsInThisRow);
    //    }
    //
    //    return rectsPerRow;
    //}

    //IntRectVec_t Factory::flattenDoubleWidthWalls_Orig(
    //    const IntRectVec_t & rectsOrig,
    //    const Spec & spec,
    //    const sf::IntRect & bounds,
    //    ColCountPerRowVec_t & colCountPerRow)
    //{
    //    if ((rectsOrig.size() < 2) || colCountPerRow.empty() || (spec.features &
    //Feature::RectsSquare))
    //    {
    //        return rectsOrig;
    //    }
    //
    //    IntRectVec_t rects;
    //    rects.reserve(rects.size());
    //
    //    for (std::size_t i(0); i < rects.size(); ++i)
    //    {
    //        sf::IntRect & rect{ rects[i] };
    //
    //        const bool isLast{ (rects.size() - 1) == i };
    //
    //        const bool isLastOfRow{ (isLast) ? true : (rect.top != rects[i + 1].top) };
    //        if (!isLastOfRow)
    //        {
    //            const sf::IntRect & nextRect{ rects[i + 1] };
    //
    //            const int rectRight{ util::right(rect) - 1 };
    //            const int nextRectLeft{ nextRect.left };
    //            if (rectRight != nextRectLeft)
    //            {
    //                rect.width += (nextRectLeft - rectRight);
    //            }
    //        }
    //
    //        if (isLastOfRow && (util::right(rect) == (bounds.width - 1)))
    //        {
    //            rect.width++;
    //        }
    //
    //        if (!isLast)
    //        {
    //            std::size_t firstOfNextRowIndex{ i };
    //
    //            while (firstOfNextRowIndex < rects.size() &&
    //                   (rects[firstOfNextRowIndex].top == rect.top))
    //            {
    //                ++firstOfNextRowIndex;
    //            }
    //
    //            if ((firstOfNextRowIndex < rects.size()))
    //            {
    //                const sf::IntRect & belowRect{ rects[firstOfNextRowIndex] };
    //
    //                const int rectBottom{ util::bottom(rect) - 1 };
    //                const int belowRectTop{ belowRect.top };
    //                if (rectBottom != belowRectTop)
    //                {
    //                    rect.height += (belowRectTop - rectBottom);
    //                }
    //            }
    //        }
    //
    //        const bool isLastOfCol{ (isLast) ? true : (rect.left != rects[i + 1].left) };
    //        if (isLastOfCol && (util::bottom(rect) == (bounds.height - 1)))
    //        {
    //            rect.height++;
    //        }
    //    }
    //
    //    return rects;
    //}

void Factory::moveTilesTopLeftUntilWallsOverlap(Job & job)
{
    if (job.result.rects.empty())
    {
        return;
    }

    sf::Vector2i tilePos(util::position(job.bounds));
    for (IntRectVec_t & row : job.result.rects)
    {
        tilePos.x = job.bounds.left;

        for (sf::IntRect & rect : row)
        {
            rect.left = tilePos.x;
            rect.top = tilePos.y;

            tilePos.x += (rect.width - 1);
        }

        tilePos.y += (row.front().height - 1);
    }
}

// void Factory::flattenDoubleWidthWalls_ForSquareTiles(
//    IntRectVecVec_t & rectsPerRow, const Spec & spec, const sf::IntRect & bounds)
//{
//    // verify params given
//    if (!(spec.features & Feature::RectsSquare) || rectsPerRow.empty() ||
//        (rectsPerRow.at(0).size() == 0))
//    {
//        return;
//    }
//
//    // verify the first rect is there and square
//    const sf::IntRect & firstRectOrig{ rectsPerRow.at(0).at(0) };
//    M_CHECK(firstRectOrig.width > 0);
//    M_CHECK(firstRectOrig.width == firstRectOrig.height);
//    const int squareSizeOrig{ firstRectOrig.width };
//
//    // find the min and max column count
//    const auto minMaxIterPair{ std::minmax_element(
//        std::begin(rectsPerRow),
//        std::end(rectsPerRow),
//        [](const IntRectVec_t & left, const IntRectVec_t & right) {
//            return (left.size() < right.size());
//        }) };
//
//    // const int minColCount{ static_cast<int>(minMaxIterPair.first->size()) };
//    int maxColCount{ static_cast<int>(minMaxIterPair.second->size()) };
//
//    // check if history has the solution
//    // TODO
//
//    // slide all as far left and up as they can go
//    for (std::size_t rowIndex(0); rowIndex < rectsPerRow.size(); ++rowIndex)
//    {
//        IntRectVec_t & rowRects{ rectsPerRow.at(rowIndex) };
//        const int posTop{ bounds.top + (static_cast<int>(rowIndex) * (squareSizeOrig - 1)) };
//
//        for (std::size_t colIndex(0); colIndex < rowRects.size(); ++colIndex)
//        {
//            sf::IntRect & rect{ rowRects.at(colIndex) };
//
//            const int posLeft{ bounds.left +
//                               (static_cast<int>(colIndex) * (squareSizeOrig - 1)) };
//
//            rect.left = posLeft;
//            rect.top = posTop;
//        }
//    }
//
//    // grow as much as possible while still keeping all square
//    int rowCount{ static_cast<int>(rectsPerRow.size()) };
//
//    const sf::Vector2i totalSizeAfterMove(
//        ((maxColCount * (squareSizeOrig - 1)) + 1), ((rowCount * (squareSizeOrig - 1)) + 1));
//
//    const sf::Vector2i freeSpaceAfterMove{ util::size(bounds) - totalSizeAfterMove };
//
//    const bool isFreeSpaceEnoughToAttemptGrowth{ (freeSpaceAfterMove.x >= maxColCount) &&
//                                                 (freeSpaceAfterMove.y >= rowCount) };
//
//    if (isFreeSpaceEnoughToAttemptGrowth)
//    {
//        auto attemptToGrow = [&](IntRectVecVec_t & rectsCopy) {
//            for (std::size_t rowIndex(0); rowIndex < rectsCopy.size(); ++rowIndex)
//            {
//                IntRectVec_t & rowRects{ rectsCopy.at(rowIndex) };
//
//                for (std::size_t colIndex(0); colIndex < rowRects.size(); ++colIndex)
//                {
//                    sf::IntRect & rect{ rowRects.at(colIndex) };
//
//                    rect.width += 1;
//                    rect.height += 1;
//                    rect.left += static_cast<int>(colIndex);
//                    rect.top += static_cast<int>(rowIndex);
//
//                    M_CHECK(rect.width == rect.height);
//
//                    if ((util::right(rect) > bounds.width) ||
//                        (util::bottom(rect) > bounds.height))
//                    {
//                        return false;
//                    }
//                }
//            }
//
//            return true;
//        };
//
//        int growCount{ 0 };
//        IntRectVecVec_t rectsPerRowCopy{ rectsPerRow };
//        while (attemptToGrow(rectsPerRowCopy))
//        {
//            ++growCount;
//            rectsPerRow = rectsPerRowCopy;
//        }
//    }
//
//    const int squareSizeFinal{ rectsPerRow.front().front().width };
//
//    const sf::Vector2i totalSizeAfterGrow(
//        (maxColCount * (squareSizeFinal - 1)) + 1, (rowCount * (squareSizeFinal - 1)) + 1);
//
//    const sf::Vector2i freeSpaceAfterGrow{ util::size(bounds) - totalSizeAfterGrow };
//
//    // add more tiles if enough free space was cleared up
//    if (spec.features & Feature::CountCanIncr)
//    {
//        const sf::Vector2i tilesToAddCount{ freeSpaceAfterGrow / (squareSizeFinal - 1) };
//
//        if ((freeSpaceAfterGrow.x > 0) && (tilesToAddCount.x > 0))
//        {
//            maxColCount += tilesToAddCount.x;
//
//            for (IntRectVec_t & rowRects : rectsPerRow)
//            {
//                for (int i(0); i < tilesToAddCount.x; ++i)
//                {
//                    sf::IntRect & newRect{ rowRects.emplace_back(rowRects.back()) };
//                    newRect.left += (squareSizeFinal - 1);
//                }
//            }
//        }
//
//        if ((freeSpaceAfterGrow.y > 0) && (tilesToAddCount.y > 0))
//        {
//            rowCount += tilesToAddCount.y;
//
//            for (int i(0); i < tilesToAddCount.y; ++i)
//            {
//                IntRectVec_t bottomRow{ rectsPerRow.back() };
//
//                for (sf::IntRect & rect : bottomRow)
//                {
//                    rect.top += (squareSizeFinal - 1);
//                }
//
//                rectsPerRow.push_back(bottomRow);
//            }
//        }
//    }
//
//    const sf::Vector2i totalSizeAfterAdding(
//        ((maxColCount * (squareSizeFinal - 1)) + 1), ((rowCount * (squareSizeFinal - 1)) +
//        1));
//
//    const sf::Vector2i freeSpaceAfterAdding{ util::size(bounds) - totalSizeAfterAdding };
//
//    // std::cout << "\nSquareTileGrowth: x" << spec.count;
//    // std::cout << ", bounds=" << bounds;
//    // std::cout << ", cols/rows=" << maxColCount << 'x' << rowCount;
//    // std::cout << ", size=" << totalSizeAfterAdding;
//    // std::cout << ", free=" << freeSpaceAfterAdding;
//    // std::cout << "\n";
//
//    // spread out evenly
//    const sf::Vector2i blockOffset{ (freeSpaceAfterAdding / 2) };
//    if ((blockOffset.x > 0) || (blockOffset.y > 0))
//    {
//        for (IntRectVec_t & rowRects : rectsPerRow)
//        {
//            for (sf::IntRect & rect : rowRects)
//            {
//                rect.left += blockOffset.x;
//                rect.top += blockOffset.y;
//            }
//        }
//    }
//}

//std::cout << "\n + + + + + + + + + SquareTileGrowth: x" << spec.count;
//std::cout << ", bounds=" << bounds;
//std::cout << ", cols_rows=" << colCountMax << 'x' << rowCount;
//
//IntRectVec_t rectsCopy{ rects };
//while ((totalSizeAfterChanges.x < bounds.width) &&
//       (totalSizeAfterChanges.y < bounds.height))
//{
//    std::cout << ", tile_size_#" << growCount << "=" << rects.front().width;
//    std::cout << ", free_space=" << (util::size(bounds) - totalSizeAfterChanges);
//
//    std::cout << ", grow_#=" << growCount << "=";
//    if (growCount > 0)
//    {
//        rects = rectsCopy;
//        std::cout << "true\n";
//    }
//    else
//    {
//        std::cout << "false\n";
//    }
//
//    ++growCount;
//    rectsIndex = 0;
//    totalSizeAfterChanges = { 0, 0 };
//
//    for (std::size_t rowIndex(0); rowIndex < colCountPerRow.size(); ++rowIndex)
//    {
//        const int colsInThisRow{ colCountPerRow.at(rowIndex) };
//
//        for (int colIndex(0); colIndex < colsInThisRow; ++colIndex)
//        {
//            M_CHECK(rectsIndex < rectsCopy.size());
//            sf::IntRect & rectToGrow{ rectsCopy[rectsIndex++] };
//
//            rectToGrow.left += colIndex;
//            rectToGrow.top += static_cast<int>(rowIndex);
//            rectToGrow.width += 1;
//            rectToGrow.height += 1;
//            M_CHECK(rectToGrow.width == rectToGrow.height);
//
//            totalSizeAfterChanges.x =
//                std::max(totalSizeAfterChanges.x, util::right(rectToGrow));
//
//            totalSizeAfterChanges.y =
//                std::max(totalSizeAfterChanges.y, util::bottom(rectToGrow));
//        }
//    }
//};
//
////
//const int squareSizeFinal{ rects.front().width };
//const sf::Vector2i extraSpace{ util::size(bounds) - totalSizeAfterChanges };
//const sf::Vector2i extraTiles{ extraSpace / (squareSizeFinal - 1) };
//
//if (extraTiles.x > 0)
//{
//    for (int i(0); i < (rowCount *); ++rowIndex)
//    {
//        const int posTop{ ((totalSizeAfterChanges.y - 1) - squareSizeFinal) +
//                          (rowIndex * (squareSizeFinal - 1)) };
//
//        for (int colIndex(0); colIndex < extraTiles.x; ++colIndex)
//        {
//            sf::IntRect & rect{ rects.emplace_back(rects[0]) };
//
//            rect.left = { (totalSizeAfterChanges.x - 1) +
//                          (colIndex * (squareSizeFinal - 1)) };
//
//            rect.top = posTop;
//
//            rect.width = squareSizeFinal;
//            rect.height = squareSizeFinal;
//        }
//    }
//}
//
//// bottom row tiles
//IntRectVec_t bottomRowRects;
//bottomRowRects.reserve(static_cast<std::size_t>(colCountMax));
//
//std::copy_if(
//    std::begin(rects),
//    std::end(rects),
//    std::back_inserter(bottomRowRects),
//    [&](const sf::IntRect & rect) { return (rect.top == rects.back().top); });
//
//if (extraTiles.y > 0)
//{
//    for (int i(0); i < extraTiles.y; ++i)
//    {
//        const int posTop{ (util::bottom(rects.back()) - 1) + (i * (squareSizeFinal - 1)) };
//
//        for (const sf::IntRect & rect : bottomRowRects)
//        {
//            rects.emplace_back(rect.left, posTop, squareSizeFinal, squareSizeFinal);
//        }
//
//        colCountPerRow.push_back(static_cast<int>(bottomRowRects.size()));
//    }
//}
//
//std::cout << '\n';

// int colIndex{0};
// setup(Orig = -1; // any negative will work here
// for (std::size_t index(0); index < rects.size(); ++index)
//{
//    sf::IntRect & rectToMove{ rects[index] };
//
//    const bool isFirstOfNewRow{ (0 == index) || (prevTopOrig < rectToMove.top) };
//    prevTopOrig = rectToMove.top;
//
//    rectToMove.width += growth;
//    rectToMove.height += growth;
//    rectToMove.left += (colIndex * growth);
//    rectToMove.top += (colIndex * growth);
//
//    if (isFirstOfNewRow)
//    {
//        ++colIndex;
//    }
//}

// grow as much as possible
// TODO

// center
// TODO
// } // namespace snake::tile

// IntRectVec_t Factory::flattenDoubleWidthWalls_ForSquareTiles(
//    const IntRectVec_t & rectsOrig,
//    const Spec &,
//    const sf::IntRect & bounds,
//    const ColCountPerRowVec_t & colCountPerRow)
//{
//    if ((rectsOrig.size() < 2) || colCountPerRow.empty())
//    {
//        return rectsOrig;
//    }
//
//    std::vector<SquaresRow> rows;
//    rows.reserve(colCountPerRow.size());
//
//    std::size_t rectsIndex{ 0 };
//    const int rowCount{ static_cast<int>(colCountPerRow.size()) };
//
//    for (int rowIndex(0); rowIndex < rowCount; ++rowIndex)
//    {
//        SquaresRow & squaresRow{ rows.emplace_back({}, rowIndex) };
//
//        const int colsInThisRowCount{ colCountPerRow[rowIndex] };
//        M_CHECK(colsInThisRowCount > 0);
//
//        const sf::IntRect firstRectOrigCopy{ rectsOrig.at(rectsIndex) };
//        for (int colIndex(0); colIndex < colsInThisRowCount; ++colIndex)
//        {
//            const sf::IntRect & rect{ squaresRow.rects.emplace_back(
//                rectsOrig.at(rectsIndex++)) };
//
//            M_CHECK(rect.width > 0);
//            M_CHECK(rect.height > 0);
//            M_CHECK(rect.width == rect.height);
//            M_CHECK(rect.top == firstRectOrigCopy.top);
//            M_CHECK(rect.width == firstRectOrigCopy.width);
//            M_CHECK(rect.height == firstRectOrigCopy.height);
//        }
//
//        const int overlapShrinkHoriz{ std::clamp((firstRectOrigCopy.width - 1), 0, 2) };
//        squaresRow.overlap_size.x = { firstRectOrigCopy.width - overlapShrinkHoriz };
//
//        squaresRow.total_free.x = bounds.width;
//        squaresRow.total_free.x -= (colsInThisRowCount * firstRectOrigCopy.width);
//        squaresRow.total_free.x += (colsInThisRowCount * overlapShrinkHoriz);
//        M_CHECK(squaresRow.total_free.x >= 0);
//
//        //
//        const int rowTop{ (0 == rowIndex) ? bounds.top
//                                          : util::bottom(rows.at(rowIndex -
//                                          1).rects.front())
//                                          };
//
//        const int rowBottom{ (rectsIndex < rectsOrig.size()) ?
//        rectsOrig.at(rectsIndex).top
//                                                             : bounds.top };
//
//        const int rowHeight{ rowBottom - rowTop };
//        M_CHECK(rowHeight >= 0);
//        M_CHECK(rowHeight >= firstRectOrigCopy.height);
//
//        const int overlapShrinkVert{ std::clamp((firstRectOrigCopy.height - 1), 0, 2) };
//        squaresRow.overlap_size.y = (firstRectOrigCopy.height - overlapShrinkVert);
//
//        squaresRow.total_free.y = rowHeight;
//        squaresRow.total_free.y -= (rowHeight - firstRectOrigCopy.height) - 1);
//    }
//
//    // sanity checks
//    M_CHECK(colCountPerRow.size() == rows.size());
//    //
//    for (std::size_t i(0); i < descs.size(); ++i)
//    {
//        const TileDesc & desc{ descs[i] };
//
//        const TileDescOpt_t prevDescOpt{ (i > 0) ? TileDescOpt_t{ descs[i - 1] }
//                                                 : TileDescOpt_t{ std::nullopt } };
//
//        const TileDescOpt_t nextDescOpt{ (i < (descs.size() - 1))
//                                             ? TileDescOpt_t{ descs[i + 1] }
//                                             : TileDescOpt_t{ std::nullopt } };
//
//        if (desc.isFirst)
//        {
//            M_CHECK(!desc.isLast);
//            M_CHECK(desc.isColFirst && desc.isRowFirst);
//            M_CHECK(!prevDescOpt && nextDescOpt);
//            M_CHECK((desc.col == 0) && (desc.row == 0));
//        }
//
//        if (desc.isLast)
//        {
//            M_CHECK(!desc.isFirst);
//            M_CHECK(desc.isColLast && desc.isRowLast);
//            M_CHECK(prevDescOpt && !nextDescOpt);
//            M_CHECK(desc.row == (rowCount - 1));
//        }
//
//        if (desc.isColFirst)
//        {
//            M_CHECK(desc.col == 0);
//            M_CHECK(desc.isFirst || nextDescOpt);
//        }
//
//        if (desc.isColLast)
//        {
//            M_CHECK(desc.isLast || nextDescOpt);
//
//            if (!desc.isColFirst)
//            {
//                M_CHECK(desc.col > 0);
//            }
//        }
//
//        if (desc.isRowFirst)
//        {
//            M_CHECK(desc.row == 0);
//        }
//
//        if (desc.isRowLast)
//        {
//            M_CHECK(desc.isLast || nextDescOpt);
//
//            if (!desc.isRowFirst)
//            {
//                M_CHECK(desc.row > 0);
//            }
//        }
//
//        if (prevDescOpt)
//        {
//            M_CHECK(!prevDescOpt->isLast);
//            M_CHECK(prevDescOpt->row <= desc.row);
//
//            if (prevDescOpt->col < desc.col)
//            {
//                M_CHECK(prevDescOpt->row == desc.row);
//                M_CHECK(!prevDescOpt->isColLast);
//            }
//            else if (prevDescOpt->col == desc.col)
//            {
//                M_CHECK(prevDescOpt->row == (desc.row - 1));
//
//                M_CHECK(prevDescOpt->isColFirst && prevDescOpt->isColLast);
//                M_CHECK(desc.isColFirst);
//            }
//            else if (prevDescOpt->col > desc.col)
//            {
//                M_CHECK(prevDescOpt->row == (desc.row - 1));
//                M_CHECK(!prevDescOpt->isColFirst && prevDescOpt->isColLast)
//                M_CHECK(desc.isColFirst);
//            }
//        }
//    }
//}

IntRectOpt_t
    Factory::makeIntTileFromFloatTile(const sf::FloatRect & tileRect, const sf::IntRect & bounds)
{
    using namespace util;

    sf::IntRect boardRect{ tileRect };

    // auto correct any minor bounds crossing
    if (boardRect.left == (bounds.left - 1))
    {
        boardRect.left = bounds.left;
    }

    if (boardRect.top == (bounds.top - 1))
    {
        boardRect.top = bounds.top;
    }

    if (right(boardRect) == (right(bounds) + 1))
    {
        boardRect.width -= 1;
    }

    if (bottom(boardRect) == (bottom(bounds) + 1))
    {
        boardRect.height -= 1;
    }

    if ((boardRect.left < 0) || (boardRect.top < 0) || (boardRect.width <= 0) ||
        (boardRect.height <= 0) || (util::right(boardRect) > util::right(bounds)) ||
        (util::bottom(boardRect) > util::bottom(bounds)))
    {
        return std::nullopt;
    }

    return boardRect;
}

// ColCountPerRowVec_t Factory::makeRowsAndCols_uniform(const Spec & spec, const sf::FloatRect
// &)
//{
//    // can assume the caller ensures spec.count > 1
//
//    // attempt one column case
//    if (spec.features & Feature::OneColumn)
//    {
//        return ColCountPerRowVec_t(static_cast<std::size_t>(spec.count), 1);
//        // const bool isOneColumnByShape{ (bounds.width / bounds.height) <
//                               (1.0f / static_cast<float>(spec.count / 2)) };
//
// const bool isOneColumnBySpec{ (spec.features & Feature::OneColumn) != 0 };
//
// if (isOneColumnByShape || isOneColumnBySpec)
//{
//    return ColCountPerRowVec_t(static_cast<std::size_t>(spec.count), 1);
//}
//
// if (spec.features & Feature::OneColumn)
//{
//    return {};
//}
//    }
//
//    // attempt one row case
//    if (spec.features & Feature::OneRow)
//    {
//        return ColCountPerRowVec_t(1_st, spec.count);
//        // const bool isOneRowByShape{ (bounds.height / bounds.width) <
//                            (1.0f / static_cast<float>(spec.count / 2)) };
//
// const bool isOneRowBySpec{ (spec.features & Feature::OneRow) != 0 };
//
// if (isOneRowByShape || isOneRowBySpec)
//{
//    return ColCountPerRowVec_t(1_st, spec.count);
//}
//
// if (spec.features & Feature::OneRow)
//{
//    return {};
//}
//    }
//
//    // attempt same number of rows as columns (perfect square)
//    if (squareroots::isPerfectSquare(spec.count))
//    {
//        const int root{ static_cast<int>(std::sqrt(spec.count)) };
//        return ColCountPerRowVec_t(static_cast<std::size_t>(root), root);
//    }
//
//    // attempt "off-by-1 imperfect square" or abs(rows-cols)==1
// if (spec.features & Feature::ColCountSame)
//    {
//        const int nextPerfectCount{ squareroots::findNextSquare(spec.count, true) };
//        const int nextUniformCount{ squareroots::findNextSquare(spec.count, false) };
//
//        int rowCount{ 0 };
//        int colCount{ 0 };
//
//        if (nextPerfectCount < nextUniformCount)
//        {
//            rowCount = static_cast<int>(std::sqrt(nextPerfectCount));
//            colCount = rowCount;
//        }
//        else
//        {
//            rowCount = static_cast<int>(std::sqrt(nextUniformCount));
//            colCount = (rowCount + 1);
//        }
//
//        // if ((spec.features & Feature::CountCanIncr) || ((rowCount * colCount) ==
// spec.count))
//        {
//            return ColCountPerRowVec_t(static_cast<std::size_t>(rowCount), colCount);
//        }
//    }
//
//    // return {};
//}

ColCountPerRowVec_t Factory::makeRowsAndCols_uniform(const Spec & spec)
{
    // can assume the caller ensures spec.count > 1

    if (spec.count == 240)
    {
        std::cout << "COUNT IS !!!.  Um..  What now?\n";
    }

    if (spec.features & Feature::OneColumn)
    {
        return ColCountPerRowVec_t(static_cast<std::size_t>(spec.count), 1);
    }
    else if (spec.features & Feature::OneRow)
    {
        return ColCountPerRowVec_t(1_st, spec.count);
    }
    else if (squareroots::isPerfectSquare(spec.count))
    {
        const int root{ static_cast<int>(std::sqrt(spec.count)) };
        return ColCountPerRowVec_t(static_cast<std::size_t>(root), root);
    }
    else // use an "off-by-1 imperfect square" or abs(rows-cols)==1 approach that looks good
    {
        const int nextPerfectCount{ squareroots::findNextSquare(spec.count, true) };
        const int nextUniformCount{ squareroots::findNextSquare(spec.count, false) };

        std::size_t rowCount{ 0 };
        int colCount{ 0 };

        if (nextPerfectCount < nextUniformCount)
        {
            rowCount = static_cast<std::size_t>(std::sqrt(nextPerfectCount));
            colCount = static_cast<int>(rowCount);
        }
        else
        {
            rowCount = static_cast<std::size_t>(std::sqrt(nextUniformCount));
            colCount = static_cast<int>(rowCount + 1);
        }

        return ColCountPerRowVec_t(rowCount, colCount);
    }
}

// ColCountPerRowVec_t Factory::makeRowsAndCols_irregular(const int count)
//{
//    // can assume the caller ensures spec.count > 1
//
//    using namespace squareroots;
//
//    // clang-format off
//    const int prevSquare    { findPrevSquare(count, Feature::None) };
//    const int prevSquareDist{ count - prevSquare };
//    const int nextSquare    { findNextSquare(count, Feature::None) };
//    const int nextSquareDist{ nextSquare - count };
//    // clang-format on
//
//    // clang-format off
//    const bool wasAbleToFindValidSquares{
//        (prevSquare <= count)       &&
//        (nextSquare >= count)       &&
//        (prevSquare <= nextSquare)  &&
//        (prevSquareDist >= 0)       &&
//        (nextSquareDist >= 0) };
//    // clang-format on
//
//    if (!wasAbleToFindValidSquares)
//    {
//        // clang-format off
//        M_LOG_SS("Failed to find valid squares before or after:  count=" << count
//            << ", prevSquare=" << prevSquare
//            << ", prevSquareDist=" << prevSquareDist
//            << ", nextSquare=" << nextSquare
//            << ", nextSquareDist=" << nextSquareDist);
//        // clang-format on
//
//        return {};
//    }
//
//    // clang-format off
//    const bool will_use_prev { (prevSquare > 0) && (prevSquareDist <= nextSquareDist) };
//    const int count_to_use   { (will_use_prev) ? prevSquare : nextSquare              };
//    const int square_to_use  { static_cast<int>(std::sqrt(count_to_use))              };
//    const int total          { (count_to_use / square_to_use) * square_to_use         };
//    const int remaining      { static_cast<int>(count) - total                        };
//    const int rows           { count_to_use / square_to_use                           };
//    const int columns        { square_to_use                                          };
//    // clang-format on
//
//    ColCountPerRowVec_t colCountForEachRow(static_cast<std::size_t>(rows), columns);
//
//    if (remaining != 0)
//    {
//        for (std::size_t i(0); i < static_cast<std::size_t>(std::abs(remaining)); ++i)
//        {
//            if (remaining < 0)
//            {
//                --colCountForEachRow[i];
//            }
//            else
//            {
//                ++colCountForEachRow[i];
//            }
//        }
//    }
//
//    return colCountForEachRow;
//}

//

//bool Warehouse::isCloseEnoughToEqual(
//    const Spec & specLeft,
//    const Result & resultLeft,
//    const Spec & specRight,
//    const Result & resultRight)
//{
//    if (resultLeft.wall_positions == resultRight.columns) Feature::CountCanIncr) &&
//specLeft.count != spec))
//
//    return (
//        (specCurr.count == specPrev.count) && (specCurr.border == specPrev.border) &&
//        (resultCurr.shapes == resultPrev.shapes));
//}
//
//bool FactoryHistory::isRepeatAlreadyInHistory(const Spec & spec, const Result & result)
//const
//{
//    for (const std::pair<Spec, Result> & pair : valid_results)
//    {
//        if (isRepeat(spec, result, pair.first, pair.second))
//        {
//            return true;
//        }
//    }
//
//    return false;
//}
//
//bool FactoryHistory::isCompatibleRepeatAlreadyInHistory(const Job & job)
//{
//    if (!job.spec.isValid() || !job.result.columns.isValid())
//    {
//        return false;
//    }
//
//    for (const auto & [specHist, resultHist] : valid_results)
//    {
//        if ((job.spec.isBlocks() != specHist.isBlocks()) ||
//            (job.spec.isOneDimm() != specHist.isOneDimm()) ||
//            (job.spec.isRooms() != specHist.isRooms()))
//        {
//            continue;
//        }
//
//        if ((job.spec.count > specHist.count) && (job.spec.count <= resultHist.tileCount()))
//        {
//            return true;
//        }
//    }
//
//    return false;
//}
//
//std::string FactoryHistory::makeSummaryStringsAndReset(
//    const int totalAttemptedCount, bool & isNumberMatch)
//{
//    const int attemptPassCount{
//static_cast<int>(tile::Factory::history.valid_results.size()) }; const int attemptFailCount{
//static_cast<int>(tile::Factory::history.fail_count) }; const int attemptTotalFromHistory{
//attemptPassCount + attemptFailCount };
//
//    if (attemptPassCount <= 0)
//    {
//        return "(ERROR: no valid results in the history";
//    }
//
//    std::ostringstream summarySS;
//    summarySS << ", att_pass=" << attemptPassCount;
//    summarySS << ", att_fa!l=" << attemptFailCount;
//
//    if (attemptTotalFromHistory != totalAttemptedCount)
//    {
//        summarySS << "(pass+fa!l(" << attemptTotalFromHistory << ") != attempts("
//                  << totalAttemptedCount << "))";
//    }
//
//    summarySS << ", repeats(pre=" << repeat_pre_count << ", post=" << repeat_post_count <<
//")";
//
//    //
//    valid_results.erase(
//        std::remove_if(
//            std::begin(valid_results),
//            std::end(valid_results),
//            [&](const auto & pair) { return !pair.second.is_success; }),
//        std::end(valid_results));
//
//    const std::size_t historyCountSuccess{ valid_results.size() };
//    const int historyCountSuccessI{ static_cast<int>(historyCountSuccess) };
//
//    summarySS << ", after_rem_fa!il=" << historyCountSuccess;
//
//    M_CHECK_LOG_SS(
//        ((attemptPassCount - historyCountSuccessI) == 0),
//        "TILE_ERROR: Somehow " << (attemptPassCount - historyCountSuccessI)
//                               << " FAILED Tiling Results with .is_success==false made it to
//" "here, which it should not.");
//
//    //
//    valid_results.erase(
//        std::remove_if(
//            std::begin(valid_results),
//            std::end(valid_results),
//            [&](const auto & pair) { return pair.second.is_repeat; }),
//        std::end(valid_results));
//
//    const std::size_t historyCountUnique{ valid_results.size() };
//
//    summarySS << ", after_rem_repeat=" << historyCountUnique;
//
//    if (historyCountUnique != historyCountSuccess)
//    {
//        isNumberMatch = false;
//        std::cout << "(after_rem: repeat!=fa!l)";
//    }
//
//    //
//    std::string valid_list_str;
//    for (const auto & [spec, result] : valid_results)
//    {
//        const std::string resultSummaryStr{ result.toString() + "\n" };
//        valid_list_str += resultSummaryStr;
//        valid_result_strs_after.push_back(resultSummaryStr);
//    }
//
//    const bool areBothTileStringListsEqual{ valid_result_strs_before ==
//                                            valid_result_strs_after };
//
//    std::string finalStr{ summarySS.str() };
//
//    if (isNumberMatch)
//    {
//        finalStr += ", num_match=PASS";
//    }
//    else
//    {
//        finalStr += ", num_match=FAI";
//    }
//
//    // TODO remove all of this when done testing
//    M_CHECK(areBothTileStringListsEqual);
//
//    // reset
//    fail_count = 0;
//    repeat_pre_count = 0;
//    repeat_post_count = 0;
//
//    valid_results.clear();
//    valid_result_strs_before.clear();
//    valid_result_strs_after.clear();
//
//    return finalStr;
//}

// bool Factory::removeTilesUniform(IntRectVecVec_t & rects, const int toRemoveCount)
//{
//    // we need ths copy to reset to orig on error
//    const auto rectsOrigCopy{ rects };
//
//    if (rects.empty() || rects.front().empty() || rects.front().front().width < 2)
//    {
//        return false;
//    }
//
//    const sf::Vector2i sampleTileSizze{ util::size(rects.front().front()) };
//
//    return true;
//}
} // namespace snake::tile

*/