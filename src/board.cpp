// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "board.hpp"

#include "context.hpp"
#include "media.hpp"
#include "pieces.hpp"
#include "random.hpp"
#include "settings.hpp"
#include "teleport-effect.hpp"
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
        m_candleSprite = sf::Sprite();
        m_candleBlackSideQuads.clear();
        m_teleportQuads.clear();
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
        const std::size_t pieceCount{ (headCount + tailCount + foodCount + wallCount) };

        ss << "\n  heads_count      = " << headCount;
        ss << "\n  tails_count      = " << tailCount;
        ss << "\n  food_count       = " << foodCount;
        ss << "\n  walls_count      = " << wallCount;
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

    void Board::loadMap(Context & context)
    {
        reset();

        m_teleportQuads.clear();
        m_teleportQuads.reserve(1000);

        // setup candle light sprite
        m_candleSprite.setTexture(context.media.candleTexture());
        m_candleSprite.setScale(1.0f, 1.0f);
        util::setOriginToCenter(m_candleSprite);

        const float candleScaleVert{ 0.7f * (context.layout.board_bounds_f.height /
                                             m_candleSprite.getLocalBounds().height) };

        m_candleSprite.setScale(candleScaleVert, candleScaleVert);
        m_candleSprite.setColor(sf::Color::Transparent);

        //
        m_pieceVerts.reserve(context.layout.cell_count_total_st);
        m_headPieces.reserve(context.layout.cell_count_total_st);
        m_wallPieces.reserve(context.layout.cell_count_total_st);
        m_foodPieces.reserve(context.layout.cell_count_total_st);

        const LevelDetails & level{ context.game.level() };

        replaceWithNewPiece(context, Piece::Head, level.start_pos);

        // place walls
        // for (const BoardPos_t & pos : level.wall_positions)
        //{
        //    replaceWithNewPiece(context, Piece::Wall, pos);
        //}

        // place food
        BoardPosVec_t boardPositions;
        boardPositions.reserve(10);

        int currentDistance{ static_cast<int>(level.number) };

        while (boardPositions.empty() && (currentDistance > 0))
        {
            boardPositions = findFreeBoardPosAtDistance(
                context,
                currentDistance,
                DistanceRule::Outside,
                level.pickups_visible_at_start_count);

            currentDistance--;
        }
        //
        // for (const BoardPos_t & pos :
        //     BoardPosVec_t { { 5, 5 }, { 6, 5 }, { 7, 5 }, { 8, 5 }, { 9, 5 } })
        //{
        //    replaceWithNewPiece(context, Piece::Food, pos);
        //}

        //
        //    // auto pickRandomDimmPos = [&](const int dimmCellLength) {
        //    //    const float stdDevRatio{ 1.25f };
        //    //    int distance{ context.random.normalFromTo(
        //    //        1, (dimmCellLength - 2), stdDevRatio, util::Random::Option::None) };
        //    //
        //    //    const std::size_t levelWhenEdgeHelpStops{ 4 };
        //    //    if (lvl.number() < levelWhenEdgeHelpStops)
        //    //    {
        //    //        const int edgeTooClose{ 4 };
        //    //        if (distance <= edgeTooClose)
        //    //        {
        //    //            distance += 1;
        //    //            distance += (levelWhenEdgeHelpStops - lvl.number());
        //    //        }
        //    //        else if ((dimmCellLength - distance) <= edgeTooClose)
        //    //        {
        //    //            distance -= 1;
        //    //            distance -= (levelWhenEdgeHelpStops - lvl.number());
        //    //        }
        //    //    }
        //    //
        //    //    return distance;
        //    //};
        //    //
        //    // const int left{ pickRandomDimmPos(context.layout.cell_counts.x) };
        //    // const int top{ pickRandomDimmPos(context.layout.cell_counts.y) };
        //    //
        //    // replaceWithNewPiece(context, Piece::Food, BoardPos_t(left, top));

        // place random abstacles
        // for (int i(0); i < 20; ++i)
        //{
        //    placePieceAtRandomPos(context, Piece::Wall);
        //}

        // for (const TeleportWallPos & telWallPos : level.teleport_positions)
        //{
        //    setupTeleportWall(context, telWallPos);
        //}
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

        // context.teleporter.add(context, pos, piece::toColor(piece));
    }

    std::size_t Board::removePiece(Context &, const BoardPos_t & posToRemove)
    {
        // context.teleporter.remove(posToRemove);

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
                case Piece::Poison: { return erasePieceAtPosition(m_poisonPieces, posToRemove); }
               
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

    sf::Vector2i
        Board::move(Context & context, const BoardPos_t & fromPos, const BoardPos_t & toPos)
    {
        // check if walked off the board
        const BoardPosOpt_t wrapPosOpt{ context.layout.findWraparoundPos(toPos) };
        if (wrapPosOpt)
        {
            return move(context, fromPos, wrapPosOpt.value());
        }

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
        // m_poisonPieces.clear();

        for (HeadPiece & piece : m_headPieces)
        {
            piece.update(context, elapsedSec);
        }

        for (TailPiece & piece : m_tailPieces)
        {
            piece.update(context, elapsedSec);
        }

        for (FoodPiece & piece : m_foodPieces)
        {
            piece.update(context, elapsedSec);
        }

        for (WallPiece & piece : m_wallPieces)
        {
            piece.update(context, elapsedSec);
        }

        {
            static bool willLighten{ false };
            const sf::Color colorAdj(0, 0, 0, 1);

            for (sf::Vertex & vert : m_teleportQuads)
            {
                if (vert.color.r == 0)
                {
                    continue;
                }

                if (willLighten)
                {
                    vert.color += colorAdj;

                    if (vert.color.a >= 150)
                    {
                        willLighten = !willLighten;
                    }
                }
                else
                {
                    vert.color -= colorAdj;

                    if (vert.color.a <= 50)
                    {
                        willLighten = !willLighten;
                    }
                }
            }
        }

        if (!m_headPieces.empty() && (m_candleSprite.getColor().a > 0))
        {
            const sf::FloatRect headBounds{ context.layout.cellBounds(
                m_headPieces.front().position()) };

            m_candleSprite.setPosition(util::center(headBounds));

            // setup the black sides of the candle light when it doesn't cover the whole
            // board extend it past the edges of the window just in case border pad is ever
            // added

            m_candleBlackSideQuads.clear();

            const float pad{ 2.0f };
            const float doublePad{ pad * 2.0f };
            const sf::FloatRect bounds{ context.layout.board_bounds_f };

            const sf::FloatRect extendedbounds{
                (0.0f - pad), (0.0f - pad), (bounds.width + doublePad), (bounds.height + doublePad)
            };

            const float leftEdge{ m_candleSprite.getGlobalBounds().left + pad };

            const sf::FloatRect leftRect(
                extendedbounds.left,
                extendedbounds.top,
                (leftEdge - extendedbounds.left),
                extendedbounds.height);

            const float topEdge{ m_candleSprite.getGlobalBounds().top + pad };

            const sf::FloatRect topRect(
                extendedbounds.left,
                extendedbounds.top,
                extendedbounds.width,
                (topEdge - extendedbounds.top));

            const float rightEdge{ util::right(m_candleSprite.getGlobalBounds()) - pad };

            const sf::FloatRect rightRect{
                rightEdge, extendedbounds.top, extendedbounds.width, extendedbounds.height
            };

            const float bottomEdge{ util::bottom(m_candleSprite.getGlobalBounds()) - pad };

            const sf::FloatRect bottomRect{
                extendedbounds.left, bottomEdge, extendedbounds.width, extendedbounds.height
            };

            util::appendQuadVerts(leftRect, m_candleBlackSideQuads, sf::Color::Black);
            util::appendQuadVerts(topRect, m_candleBlackSideQuads, sf::Color::Black);
            util::appendQuadVerts(rightRect, m_candleBlackSideQuads, sf::Color::Black);
            util::appendQuadVerts(bottomRect, m_candleBlackSideQuads, sf::Color::Black);
        }

        //// TODO REMOVE AFTER TESTING SURROUNDING ALGS
        // for (const BoardPos_t & pos : findFreeBoardPosAroundBody(context, 4, 1, 0))
        //{
        //    replaceWithNewPiece(context, Piece::Poison, pos);
        //}
    }

    void Board::draw(
        const Context & context, sf::RenderTarget & target, const sf::RenderStates & states) const
    {
        // board region solid background color
        util::drawRectangleVerts(
            target, context.layout.board_bounds_f, context.config.board_background_color);

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

        if (m_candleSprite.getColor().a > 0)
        {
            target.draw(m_candleSprite, sf::BlendMultiply);
            target.draw(m_candleBlackSideQuads);
        }

        if (!m_teleportQuads.empty())
        {
            target.draw(&m_teleportQuads[0], m_teleportQuads.size(), sf::Quads, sf::BlendAdd);
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

    // BoardPosVec_t findFreeBoardPosAtDifficulty(
    //    const Context & context, const float difficultyRatio, const std::size_t count) const;

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
            m_headPieces.size() + m_tailPieces.size() + m_foodPieces.size() + m_wallPieces.size());
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

    PieceBase & Board::makePiece(Context & context, const Piece piece, const BoardPos_t & pos)
    {
        switch (piece)
        {
            case Piece::Head: return m_headPieces.emplace_back(HeadPiece(context, pos));
            case Piece::Tail: return m_tailPieces.emplace_front(TailPiece(context, pos)); //-V525
            case Piece::Food: return m_foodPieces.emplace_back(FoodPiece(context, pos));
            case Piece::Wall: return m_wallPieces.emplace_back(WallPiece(context, pos));
            case Piece::Poison: return m_poisonPieces.emplace_back(PoisonPiece(context, pos));

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

    // void Board::setupTeleportWall(Context & context, const TeleportWallPos & telePos)
    //{
    //    M_CHECK_SS(
    //        ((telePos.pos.x == 0) || (telePos.pos.y == 0)),
    //        "pos=" << telePos.pos << ", count=" << telePos.count);
    //
    //    M_CHECK_SS((telePos.count > 0), "count=" << telePos.count);
    //
    //    const sf::FloatRect bounds{ combineTeleportWallPieces(
    //        context, telePos.pos, telePos.count) };
    //
    //    appendTeleportLineVerts(context, bounds, telePos.pos);
    //
    //    //
    //
    //    BoardPos_t oppPos{ telePos.pos };
    //    if (oppPos.x == 0)
    //    {
    //        oppPos.x = (context.layout.cell_counts.x - 1);
    //    }
    //    else
    //    {
    //        M_CHECK_SS((oppPos.y == 0), "pos=" << telePos.pos << ", count=" << telePos.count);
    //        oppPos.y = (context.layout.cell_counts.y - 1);
    //    }
    //
    //    const sf::FloatRect oppBounds{ combineTeleportWallPieces(context, oppPos,
    //    telePos.count)
    //    }; appendTeleportLineVerts(context, oppBounds, oppPos);
    //}
    //
    // sf::FloatRect
    //    Board::combineTeleportWallPieces(Context & context, const BoardPos_t & pos, const int
    //    count)
    //{
    //    const sf::Keyboard::Key countDir{
    //        ((pos.x == 0) || (pos.x == (context.layout.cell_counts.x - 1))) ?
    //        sf::Keyboard::Down
    //                                                                        :
    //                                                                        sf::Keyboard::Right
    //    };
    //
    //    sf::VertexArray tempVerts{ sf::Quads };
    //
    //    BoardPos_t countPos{ pos };
    //    for (int cnt(0); cnt < count; ++cnt)
    //    {
    //        util::appendQuadVerts(context.layout.cellBounds(countPos), tempVerts);
    //        removePiece(context, countPos);
    //        countPos = keys::move(countPos, countDir);
    //    }
    //
    //    return tempVerts.getBounds();
    //}
    //
    // sf::Vector2f Board::appendTeleportLineVerts(
    //    const Context & context, const sf::FloatRect & piecesBounds, const BoardPos_t & pos)
    //{
    //    const sf::Color color(230, 190, 180, 100);
    //
    //    std::vector<sf::Vertex> shadowVerts;
    //    util::appendQuadVerts(piecesBounds, shadowVerts, color);
    //
    //    const float moveAmount{ 1.0f };
    //    // (context.layout.cell_size.x * 0.5f) - 1.0f};
    //
    //    sf::Vector2f move(0.0f, 0.0f);
    //
    //    if (pos.x == 0)
    //    {
    //        // line on the left, emitting effects to the right
    //        move.x = moveAmount;
    //        shadowVerts[1].color = sf::Color::Transparent;
    //        shadowVerts[2].color = sf::Color::Transparent;
    //    }
    //    else if (pos.x == (context.layout.cell_counts.x - 1))
    //    {
    //        // line on the right, emitting effects to the left
    //        move.x -= moveAmount;
    //        shadowVerts[0].color = sf::Color::Transparent;
    //        shadowVerts[3].color = sf::Color::Transparent;
    //    }
    //    else if (pos.y == 0)
    //    {
    //        // line on the top, emitting effects down
    //        move.y = moveAmount;
    //        shadowVerts[2].color = sf::Color::Transparent;
    //        shadowVerts[3].color = sf::Color::Transparent;
    //    }
    //    else if (pos.y == (context.layout.cell_counts.y - 1))
    //    {
    //        // line on the bottom, emitting effects up
    //        move.y -= moveAmount;
    //        shadowVerts[0].color = sf::Color::Transparent;
    //        shadowVerts[1].color = sf::Color::Transparent;
    //    }
    //
    //    for (sf::Vertex & vert : shadowVerts)
    //    {
    //        vert.position += move;
    //        m_teleportQuads.push_back(vert);
    //    }
    //
    //    // for (sf::Vertex & vert : shadowVerts)
    //    //{
    //    //    vert.position -= (2.0f * move);
    //    //
    //    //    if (sf::Color::Transparent == vert.color)
    //    //    {
    //    //        vert.color = color;
    //    //    }
    //    //    else
    //    //    {
    //    //        vert.color = sf::Color::Transparent;
    //    //    }
    //    //
    //    //    m_teleportQuads.push_back(vert);
    //    //}
    //
    //    return {};
    //}
} // namespace snake
