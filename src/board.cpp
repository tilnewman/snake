// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "board.hpp"

#include "context.hpp"
#include "layout.hpp"
#include "media.hpp"
#include "pieces.hpp"
#include "random.hpp"
#include "settings.hpp"
#include "util.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace snake
{
    void Board::reset()
    {
        m_posEntryMap.clear();
        m_pieceVerts.clear();
        m_headPieces.clear();
        m_tailPieces.clear();
        m_wallPieces.clear();
        m_foodPieces.clear();
        m_slowPieces.clear();
        m_shrinkPieces.clear();

        m_pieceVerts.reserve(2000);
        m_headPieces.reserve(10);
        m_wallPieces.reserve(1000);
        m_foodPieces.reserve(100);
        m_slowPieces.reserve(10);
        m_shrinkPieces.reserve(10);
    }

    std::string Board::toString(const Context & context) const
    {
        std::ostringstream ss;

        ss << "Board:";
        ss << "\n  pos_entry_map_size      = " << m_posEntryMap.size();

        ss << "\n  piece_verts_vec: size=" << m_pieceVerts.size()
           << "/4=" << (m_pieceVerts.size() / util::verts_per_quad)
           << "/%4=" << (m_pieceVerts.size() % util::verts_per_quad);

        ss << "\n  piece_verts_vec   = " << m_pieceVerts.capacity();

        std::size_t usedCount{ 0 };
        std::size_t availCount{ 0 };
        for (std::size_t i(0); i < m_pieceVerts.size(); i += util::verts_per_quad)
        {
            if (m_pieceVerts.at(i).color == m_freeVertColor)
            {
                ++availCount;
            }
            else
            {
                ++usedCount;
            }
        }

        ss << "\n  quads_used      = " << usedCount;
        ss << "\n  quads_free      = " << (availCount / util::verts_per_quad);
        ss << "\n  m_pieceVerts.size()  = " << m_pieceVerts.size();

        const std::size_t headCount{ m_headPieces.size() };
        const std::size_t tailCount{ m_tailPieces.size() };
        const std::size_t foodCount{ m_foodPieces.size() };
        const std::size_t wallCount{ m_wallPieces.size() };
        const std::size_t slowCount{ m_slowPieces.size() };
        const std::size_t shrinkCount{ m_slowPieces.size() };

        const std::size_t pieceCount{ (
            headCount + tailCount + foodCount + wallCount + slowCount + shrinkCount) };

        ss << "\n  heads_count      = " << headCount;
        ss << "\n  tails_count      = " << tailCount;
        ss << "\n  food_count       = " << foodCount;
        ss << "\n  walls_count      = " << wallCount;
        ss << "\n  slow_count       = " << slowCount;
        ss << "\n  shrink_count     = " << shrinkCount;
        ss << "\n  all_pieces_count = " << pieceCount;

        ss << "\n  all_pieces_count SHOULD == used_vertex_count AND SHOULD == "
              "map_pos_count: "
           << pieceCount << '/' << usedCount << '/' << m_posEntryMap.size();

        for (const auto & [pos, entry] : m_posEntryMap)
        {
            if (!context.layout.isPositionValid(pos))
            {
                ss << "\n     pos   = " << pos
                   << " is out of bounds=" << context.layout.cell_counts;
            }

            if (!isQuadIndexValid(entry.quad_index))
            {
                ss << "\n     pos   = " << pos << "(INVALID:verts_vec_size=" << m_pieceVerts.size()
                   << ")";
            }

            if (isQuadFree(entry.quad_index))
            {
                // clang-format off
                ss << "(VERTS/QUAD_NOT_AVAILABLE/Transparent:\n" <<
                    "\n     index   = " << (entry.quad_index + 0) << "=" << m_pieceVerts.at(entry.quad_index + 0).color << ", " <<
                    "\n     index   = " << (entry.quad_index + 1) << "=" << m_pieceVerts.at(entry.quad_index + 1).color << ", " <<
                    "\n     index   = " << (entry.quad_index + 2) << "=" << m_pieceVerts.at(entry.quad_index + 2).color << ", " <<
                    "\n     index   = " << (entry.quad_index + 3) << "=" << m_pieceVerts.at(entry.quad_index + 3).color;
                // clang-format on
            }
        }

        return ss.str();
    }

    void Board::loadMap(Context & context, const bool willLoadNewMap)
    {
        if (willLoadNewMap)
        {
            reset();

            const LevelDetails & level{ context.game.level() };

            replaceWithNewPiece(context, Piece::Head, level.start_pos);

            // place walls
            if (context.random.boolean())
            {
                for (const BoardPos_t & pos : level.wall_positions)
                {
                    replaceWithNewPiece(context, Piece::Wall, pos);
                }
            }

            // place random obstacles
            if (context.random.boolean())
            {
                for (std::size_t i(0); i < (context.game.level().number * 2); ++i)
                {
                    addNewPieceAtRandomFreePos(context, Piece::Wall);
                }
            }

            // place food
            if (context.random.boolean())
            {
                const std::size_t foodCount =
                    context.random.fromTo(1_st, context.game.level().remainingToEat());

                for (std::size_t i(0); i < foodCount; ++i)
                {
                    addNewPieceAtRandomFreePos(context, Piece::Food);
                }
            }
        }
        else
        {
            removeAllPieces(context, Piece::Head);
            removeAllPieces(context, Piece::Tail);
            removeAllPieces(context, Piece::Food);
            removeAllPieces(context, Piece::Shrink);
            removeAllPieces(context, Piece::Slow);

            replaceWithNewPiece(context, Piece::Head, context.game.level().start_pos);
        }
    }

    bool Board::isPiece(const BoardPos_t & pos, const Piece piece) const
    {
        const PosEntryOpt_t entryOpt{ entryAt(pos) };
        return (entryOpt.has_value() && (entryOpt->piece_enum == piece));
    }

    PieceEnumOpt_t Board::pieceEnumOptAt(const BoardPos_t & pos) const
    {
        const PosEntryOpt_t entryOpt{ entryAt(pos) };
        if (entryOpt)
        {
            return entryOpt->piece_enum;
        }
        else
        {
            return std::nullopt;
        }
    }

    void Board::addNewPieceAtRandomFreePos(Context & context, const Piece piece)
    {
        const BoardPosOpt_t posOpt{ findFreeBoardPosRandom(context) };

        if (!posOpt)
        {
            M_LOG_SS(
                "Unable to find any free/available/open positions on the board!\n"
                << context.layout.toString() << "\n"
                << toString(context));

            return;
        }

        replaceWithNewPiece(context, piece, posOpt.value());
    }

    void Board::replaceWithNewPiece(Context & context, const Piece piece, const BoardPos_t & pos)
    {
        M_CHECK_SS(context.layout.isPositionValid(pos), pos);

        std::size_t quadIndex{ removePiece(context, pos) };
        if (!isQuadIndexValid(quadIndex) || m_pieceVerts.empty())
        {
            quadIndex = findOrMakeFreeQuadIndex();
        }

        M_CHECK_SS(isQuadIndexValid(quadIndex), quadIndex);
        M_CHECK_SS(isQuadFree(quadIndex), quadIndex);

        setupQuad(context, quadIndex, pos, piece::toColor(piece));
        M_CHECK_SS(!isQuadFree(quadIndex), entryToString(PosEntry(piece, quadIndex)));

        makePiece(context, piece, pos);
        m_posEntryMap.erase(pos);
        m_posEntryMap.insert({ pos, PosEntry(piece, quadIndex) });

        M_CHECK_SS(entryAt(pos).has_value(), pos);
        M_CHECK_SS((entryAt(pos)->piece_enum == piece), entryAt(pos)->piece_enum);
    }

    std::size_t Board::removePiece(Context &, const BoardPos_t & posToRemove)
    {
        auto erasePieceAtPosition = [&](auto & cont, const BoardPos_t & posTemp) {
            const std::size_t countBefore{ cont.size() };

            cont.erase(
                std::remove_if(
                    std::begin(cont),
                    std::end(cont),
                    [&](const auto & piece) { return (piece.position() == posTemp); }),
                std::end(cont));

            const std::size_t countAfter{ cont.size() };

            return (countBefore - countAfter);
        };

        auto erasePieceInContainer = [&](const Piece piece) {
            // clang-format off
            switch (piece)
            {
                case Piece::Head:   { return erasePieceAtPosition(m_headPieces, posToRemove); }
                case Piece::Tail:   { return erasePieceAtPosition(m_tailPieces, posToRemove); }
                case Piece::Food:   { return erasePieceAtPosition(m_foodPieces, posToRemove); }
                case Piece::Wall:   { return erasePieceAtPosition(m_wallPieces, posToRemove); }
                case Piece::Slow:   { return erasePieceAtPosition(m_slowPieces, posToRemove); }
                case Piece::Shrink: { return erasePieceAtPosition(m_shrinkPieces, posToRemove); }
                default: { break; }
            }
            // clang-format on

            std::ostringstream ss;

            ss << "Board::removePiece(posToRemove=" << posToRemove
               << ") called lambda erasePieceInContainer(piece=" << piece
               << " -but that piece enum is unknown.";

            throw std::runtime_error(ss.str());
        };

        const auto entryToRemoveIter = m_posEntryMap.find(posToRemove);
        if (entryToRemoveIter == std::end(m_posEntryMap))
        {
            return m_pieceVerts.size();
        }

        const PosEntry entryToRemoveCopy{ entryToRemoveIter->second };
        freeQuad(entryToRemoveCopy.quad_index);

        const std::size_t piecesErasedCount{ erasePieceInContainer(entryToRemoveCopy.piece_enum) };

        M_CHECK_LOG_SS(
            (piecesErasedCount == 1),
            "WARNING:  posToRemove=" << posToRemove << ", erased " << piecesErasedCount);

        m_posEntryMap.erase(entryToRemoveIter);
        return entryToRemoveCopy.quad_index;
    }

    std::size_t Board::removeAllPieces(Context & context, const Piece piece)
    {
        const std::vector<BoardPos_t> positions = findPieces(piece);

        for (const BoardPos_t & pos : positions)
        {
            removePiece(context, pos);
        }

        return positions.size();
    }

    sf::Vector2i
        Board::move(Context & context, const BoardPos_t & fromPos, const BoardPos_t & toPos)
    {
        M_CHECK_SS(
            entryAt(fromPos),
            "fromPos=" << fromPos << ", toPos=" << toPos
                       << " -but screw that because we're trying to move a fromPos that no piece "
                          "is at...");

        const PosEntry fromEntryCopyBefore = [&]() {
            const auto fromOrigIter{ m_posEntryMap.find(fromPos) };

            M_CHECK_SS(
                (m_posEntryMap.find(fromPos) != std::end(m_posEntryMap)),
                "fromPos="
                    << fromPos << ", toPos=" << toPos
                    << " -but screw that because we're trying to move a fromPos that no piece "
                       "is at...");

            removePiece(context, toPos);

            return fromOrigIter->second;
        }();

        m_posEntryMap.insert({ toPos, fromEntryCopyBefore });
        m_posEntryMap.erase(fromPos);

        setupQuad(context, fromEntryCopyBefore.quad_index, toPos);

        M_CHECK_SS(!entryAt(fromPos).has_value(), entryToString(entryAt(fromPos).value()));

        M_CHECK_SS(
            (entryAt(toPos).has_value() &&
             (entryAt(toPos)->piece_enum == fromEntryCopyBefore.piece_enum)),
            entryToString(entryAt(fromPos).value()));

        return toPos;
    }

    void Board::update(Context & context, const float elapsedSec)
    {
        for (HeadPiece & piece : m_headPieces)
        {
            piece.update(context, elapsedSec);
        }

        for (TailPiece & piece : m_tailPieces)
        {
            piece.update(context, elapsedSec);
        }
    }

    void Board::draw(
        const Context & context, sf::RenderTarget & target, const sf::RenderStates & states) const
    {
        // board region outline
        sf::FloatRect outlineRect = context.layout.board_bounds_f;
        outlineRect.left -= 1.0f;
        outlineRect.top -= 1.0f;
        outlineRect.width += 2.0f;
        outlineRect.height += 2.0f;

        util::drawRectangleShape(
            target,
            outlineRect,
            false,
            context.config.alt_board_background_color + sf::Color(25, 25, 25));

        // draw every other cell with a slightly brighter color for a nice looking checker pattern
        const auto & cellVerts{ context.layout.cellVerts() };
        if (!cellVerts.empty())
        {
            target.draw(&cellVerts[0], cellVerts.size(), sf::Quads, states);
        }

        if (!m_pieceVerts.empty())
        {
            target.draw(&m_pieceVerts[0], m_pieceVerts.size(), sf::Quads, states);
        }
    }

    void Board::passEventToPieces(Context & context, const sf::Event & event)
    {
        // only head pieces can respond to events
        for (HeadPiece & piece : m_headPieces)
        {
            piece.handleEvent(context, event);
        }
    }

    BoardPosVec_t Board::findAllFreePositions(const Context & context) const
    {
        // start with a copy of all valid/on-board positions
        std::set<BoardPos_t> positions{ context.layout.allValidPositions() };

        // remove any that are alraedy occupied
        for (const auto & [pos, entry] : m_posEntryMap)
        {
            positions.erase(pos);
        }

        BoardPosVec_t freePositions;
        freePositions.reserve(positions.size());
        for (const BoardPos_t & pos : positions)
        {
            freePositions.push_back(pos);
        }

        return freePositions;
    }

    BoardPosOpt_t Board::findFreeBoardPosRandom(const Context & context) const
    {
        // start with a copy of all valid/on-board positions
        std::set<BoardPos_t> positions{ context.layout.allValidPositions() };

        // remove any that are alraedy occupied
        for (const auto & [pos, entry] : m_posEntryMap)
        {
            positions.erase(pos);
        }

        if (positions.empty())
        {
            return std::nullopt;
        }

        return context.random.from(positions);
    }

    BoardPosVec_t Board::findFreeBoardPosAtDistance(
        const Context & context,
        const int targetDistance,
        const DistanceRule distanceRule,
        const std::size_t count) const
    {
        BoardPosVec_t finalPositions;

        // start with a copy of all valid/on-board positions
        const BoardPosVec_t allFreePositions{ findAllFreePositions(context) };
        if (allFreePositions.empty() || (targetDistance <= 0))
        {
            return finalPositions;
        }

        const BoardPos_t headPos{ m_headPieces.front().position() };

        std::multimap<int, BoardPos_t> targetDistPosistions;
        for (const BoardPos_t & pos : allFreePositions)
        {
            const int distanceFromTarget{ std::abs(pos.x - headPos.x) +
                                          std::abs(pos.y - headPos.y) };

            if (distanceRule == DistanceRule::Exact)
            {
                if (distanceFromTarget != targetDistance)
                {
                    continue;
                }
            }

            if (distanceRule == DistanceRule::Inside)
            {
                if (distanceFromTarget > targetDistance)
                {
                    continue;
                }
            }

            if (distanceRule == DistanceRule::Outside)
            {
                if (distanceFromTarget < targetDistance)
                {
                    continue;
                }
            }

            targetDistPosistions.insert({ distanceFromTarget, pos });
        }

        if (targetDistPosistions.empty())
        {
            return finalPositions;
        }

        finalPositions.reserve(targetDistPosistions.size());
        for (const auto & distPosPair : targetDistPosistions)
        {
            finalPositions.push_back(distPosPair.second);
        }

        assert(finalPositions.size() == targetDistPosistions.size());

        std::sort(std::begin(finalPositions), std::end(finalPositions));

        finalPositions.erase(
            std::unique(std::begin(finalPositions), std::end(finalPositions)),
            std::end(finalPositions));

        context.random.shuffle(finalPositions);

        if ((count > 0) && finalPositions.size() > count)
        {
            finalPositions.resize(count);
        }

        return finalPositions;
    }

    BoardPosVec_t Board::findFreeBoardPosAroundBody(
        const Context & context, const int distanceMin, const int, const std::size_t count) const
    {
        assert(distanceMin > 0);

        const std::size_t reserveCount{ static_cast<std::size_t>(context.layout.cell_count_total) };

        BoardPosVec_t innerPositions;
        innerPositions.reserve(reserveCount);

        BoardPosVec_t outerPositions;
        outerPositions.reserve(reserveCount);

        auto isPosValidAndFree = [&](const BoardPos_t & pos) {
            if (!context.layout.isPositionValid(pos) || isPieceAt(pos))
            {
                return false;
            }

            return (
                std::find(std::begin(innerPositions), std::end(innerPositions), pos) ==
                std::end(innerPositions));
        };

        // start with inner positions as all the body positions
        assert(m_headPieces.size() >= 1);
        innerPositions.push_back(m_headPieces.front().position());

        for (const TailPiece & piece : m_tailPieces)
        {
            innerPositions.push_back(piece.position());
        }

        // find free positions around the inner positions
        // minCount starts at one because inner positions already filled with valid body positions
        int minCount(1);
        for (; minCount <= distanceMin; ++minCount)
        {
            for (const BoardPos_t & bodyPos : innerPositions)
            {
                for (const sf::Vector2i & offset : surroundingsPositionOffsets)
                {
                    const BoardPos_t pos(bodyPos + offset);

                    if (isPosValidAndFree(pos))
                    {
                        outerPositions.push_back(pos);
                    }
                }
            }

            // copy all outer to inner to outerPositions and remove any duplicates
            std::copy(
                std::begin(outerPositions),
                std::end(outerPositions),
                std::back_inserter(innerPositions));

            std::sort(std::begin(innerPositions), std::end(innerPositions));

            innerPositions.erase(
                std::unique(std::begin(innerPositions), std::end(innerPositions)),
                std::end(innerPositions));

            if (minCount < distanceMin)
            {
                outerPositions.clear();
            }
        }

        if ((count > 0) && outerPositions.size() > count)
        {
            outerPositions.resize(count);
        }

        return outerPositions;
    }

    void Board::colorQuad(const BoardPos_t & pos, const sf::Color & color)
    {
        const PosEntryOpt_t entryOpt{ entryAt(pos) };
        if (!entryOpt)
        {
            return;
        }

        colorQuad(entryOpt->quad_index, color);
    }

    const PosEntryOpt_t Board::entryAt(const BoardPos_t & pos) const
    {
        const auto foundIter{ m_posEntryMap.find(pos) };
        if (foundIter == std::end(m_posEntryMap))
        {
            return std::nullopt;
        }
        else
        {
            return foundIter->second;
        }
    }

    void Board::reColorTailPieces(Context & context)
    {
        float index{ 0.0f };
        const float count{ static_cast<float>(m_tailPieces.size()) };
        for (auto iter(std::begin(m_tailPieces)); iter != std::end(m_tailPieces); ++iter)
        {
            iter->color(
                context,
                util::colorBlend((index / count), TailPiece::m_colorLight, TailPiece::m_colorDark));

            index += 1.0f;
        }
    }

    std::size_t Board::allPiecesCount() const
    {
        return (
            m_headPieces.size() + m_tailPieces.size() + m_foodPieces.size() + m_slowPieces.size() +
            m_wallPieces.size());
    }

    std::vector<BoardPos_t> Board::findPieces(const Piece piece) const
    {
        std::vector<BoardPos_t> positions;
        positions.reserve(allPiecesCount());

        for (const auto & [pos, entry] : m_posEntryMap)
        {
            if (piece == entry.piece_enum)
            {
                positions.push_back(pos);
            }
        }

        return positions;
    }

    const AdjacentInfoOpt_t
        Board::adjacentInfoOpt(const BoardPos_t & centerPos, const sf::Keyboard::Key dir) const
    {
        const BoardPos_t adjPos{ keys::move(centerPos, dir) };
        const auto enumOpt{ pieceEnumOptAt(adjPos) };
        if (enumOpt)
        {
            return AdjacentInfo{ enumOpt.value(), adjPos, dir };
        }
        else
        {
            return std::nullopt;
        }
    }

    const Surroundings Board::surroundings(const BoardPos_t & centerPos) const
    {
        Surroundings surr(centerPos);

        // clang-format off
            if (const AdjacentInfoOpt_t opt{ adjacentInfoOpt(centerPos, sf::Keyboard::Up) };    opt) surr.adjacents.push_back(opt.value());
            if (const AdjacentInfoOpt_t opt{ adjacentInfoOpt(centerPos, sf::Keyboard::Down) };  opt) surr.adjacents.push_back(opt.value());
            if (const AdjacentInfoOpt_t opt{ adjacentInfoOpt(centerPos, sf::Keyboard::Left) };  opt) surr.adjacents.push_back(opt.value());
            if (const AdjacentInfoOpt_t opt{ adjacentInfoOpt(centerPos, sf::Keyboard::Right) }; opt) surr.adjacents.push_back(opt.value());
        // clang-format on

        return surr;
    }

    void Board::shrinkTail(Context & context)
    {
        // stop growing the tail
        for (HeadPiece & headPiece : m_headPieces)
        {
            headPiece.resetTailGrowCounter();
        }

        std::size_t newTailSize = (m_tailPieces.size() / 2);

        if (newTailSize < context.game.level().tail_start_length)
        {
            newTailSize = context.game.level().tail_start_length;
        }

        while (m_tailPieces.size() > newTailSize)
        {
            removePiece(context, findLastTailPiecePos());
        }

        reColorTailPieces(context);
    }

    PieceBase & Board::makePiece(Context & context, const Piece piece, const BoardPos_t & pos)
    {
        switch (piece)
        {
            case Piece::Head: return m_headPieces.emplace_back(HeadPiece(context, pos));
            case Piece::Tail: return m_tailPieces.emplace_front(TailPiece(context, pos)); //-V525
            case Piece::Food: return m_foodPieces.emplace_back(FoodPiece(context, pos));
            case Piece::Wall: return m_wallPieces.emplace_back(WallPiece(context, pos));
            case Piece::Slow: return m_slowPieces.emplace_back(SlowPiece(context, pos));
            case Piece::Shrink: return m_shrinkPieces.emplace_back(ShrinkPiece(context, pos));
            default: break;
        }

        std::ostringstream ss;

        ss << "Board::makePiece(piece=" << piece << ", pos=" << pos
           << ") -but that piece enum is unknown.";

        throw std::runtime_error(ss.str());
    }

    std::size_t Board::findOrMakeFreeQuadIndex()
    {
        if (m_pieceVerts.size() >= util::verts_per_quad)
        {
            for (std::size_t index(0); index < (m_pieceVerts.size() - util::verts_per_quad);
                 index += util::verts_per_quad)
            {
                if (isQuadFree(index))
                {
                    return index;
                }
            }
        }

        const std::size_t freeQuadIndex{ m_pieceVerts.size() };
        m_pieceVerts.resize((m_pieceVerts.size() + util::verts_per_quad), m_freeQuadVertex);
        return freeQuadIndex;
    }

    std::string Board::entryToString(const PosEntry & entry) const
    {
        std::ostringstream ss;

        ss << "PosEntry(" << entry.piece_enum;
        ss << ", #" << entry.quad_index;
        ss << "/%4=" << (entry.quad_index % util::verts_per_quad);
        ss << "/Q#" << (entry.quad_index / util::verts_per_quad);

        for (std::size_t i(0); i < util::verts_per_quad; ++i)
        {
            ss << "\n\t" << i << "/" << (entry.quad_index + i) << ":\t"
               << m_pieceVerts.at(entry.quad_index + i);
        }

        ss << "\n\t-";

        for (const auto & piece : m_headPieces)
        {
            ss << "\n\t" << piece.piece() << ", pos=" << piece.position()
               << ", color=" << piece.color();
        }

        ss << ")";
        ss << entryInvalidDesc(entry);
        return ss.str();
    }

    std::string Board::entryInvalidDesc(const PosEntry & entry) const
    {
        std::ostringstream ss;

        const std::string pieceEnumName{ piece::toString(entry.piece_enum) };
        if (pieceEnumName.empty())
        {
            ss << "(ERROR:PIECE_" << int(entry.piece_enum) << "_HAS_EMPTY_NAME)";
        }

        const bool isMultipleOfFour{ (entry.quad_index % util::verts_per_quad) == 0 };
        if (!isMultipleOfFour)
        {
            ss << "(ERROR:INDEX_" << entry.quad_index << "_NOT_MULT_OF_4)";
        }

        const bool isIndexInRange{ (entry.quad_index + util::verts_per_quad) <=
                                   m_pieceVerts.size() };
        if (!isIndexInRange)
        {
            ss << "(ERROR:INDEX_" << entry.quad_index
               << "_OUT_OF_RANGE_WITH_VERT_VEC_SIZE=" << m_pieceVerts.size() << ")";
        }

        if (isQuadIndexValid(entry.quad_index))
        {
            if (isQuadFree(entry.quad_index))
            {
                ss << "(COLORS_INVALID_FREE/TRANSPARENT)";
            }
        }
        else
        {
            ss << "(INDEX_IS_INVALID:vert_vec.size=" << m_pieceVerts.size() << ")";
        }

        return ss.str();
    }

    void Board::setupQuad(
        Context & context,
        const std::size_t quadIndex,
        const BoardPos_t & pos,
        const sf::Color & color)
    {
        M_CHECK_SS(isQuadIndexValid(quadIndex), quadIndex);

        const sf::FloatRect rect{ context.layout.cellBounds(pos) };
        const sf::Vector2f rectPos{ rect.left, rect.top };

        if (color != m_freeVertColor)
        {
            colorQuad(quadIndex, color);
        }

        // clang-format off
        m_pieceVerts[quadIndex + 0].position = { rectPos + sf::Vector2f(      0.0f,        0.0f) };
        m_pieceVerts[quadIndex + 1].position = { rectPos + sf::Vector2f(rect.width,        0.0f) };
        m_pieceVerts[quadIndex + 2].position = { rectPos + sf::Vector2f(rect.width, rect.height) };
        m_pieceVerts[quadIndex + 3].position = { rectPos + sf::Vector2f(      0.0f, rect.height) };
        // clang-format on
    }

    void Board::freeQuad(const std::size_t quadIndex) { colorQuad(quadIndex, m_freeVertColor); }

    void Board::colorQuad(const std::size_t quadIndex, const sf::Color & color)
    {
        M_CHECK_SS(isQuadIndexValid(quadIndex), quadIndex);

        m_pieceVerts[quadIndex + 0].color = color;
        m_pieceVerts[quadIndex + 1].color = color;
        m_pieceVerts[quadIndex + 2].color = color;
        m_pieceVerts[quadIndex + 3].color = color;
    }

    bool Board::isQuadIndexValid(const std::size_t quadIndex) const
    {
        const bool isMultipleOfFour{ (quadIndex % util::verts_per_quad) == 0 };
        const bool isIndexInRange{ (quadIndex + util::verts_per_quad) <= m_pieceVerts.size() };
        return (isMultipleOfFour && isIndexInRange);
    }

    bool Board::isQuadFree(const std::size_t quadIndex) const
    {
        M_CHECK_SS(isQuadIndexValid(quadIndex), quadIndex);

        for (std::size_t i(0); ((i < util::verts_per_quad) && (i < m_pieceVerts.size())); ++i)
        {
            if (m_pieceVerts[quadIndex + i].color != m_freeVertColor)
            {
                return false;
            }
        }

        return true;
    }

} // namespace snake
