#ifndef SNAKE_BOARD_HPP_INCLUDED
#define SNAKE_BOARD_HPP_INCLUDED
//
// board.hpp
//
#include "adjacent.hpp"
#include "check-macros.hpp"
#include "common-types.hpp"
#include "keys.hpp"
#include "pieces.hpp"

#include <array>
#include <list>
#include <optional>
#include <tuple>
#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{
    enum class DistanceRule
    {
        Exact,
        Inside,
        Outside
    };

    //

    struct PosEntry
    {
        PosEntry(const Piece piece, const std::size_t quadIndex) noexcept
            : piece_enum(piece)
            , quad_index(quadIndex)
        {}

        Piece piece_enum;
        std::size_t quad_index;
    };

    using PosEntryOpt_t = std::optional<PosEntry>;

    //

    class Board
    {
      public:
        Board() = default;

        void reset();

        void loadMap(Context & context, const bool willLoadNewMap);

        bool isPiece(const BoardPos_t & pos, const Piece piece) const;
        inline bool isPieceAt(const BoardPos_t & pos) const { return entryAt(pos).has_value(); }
        PieceEnumOpt_t pieceEnumOptAt(const BoardPos_t & pos) const;

        void addNewPieceAtRandomFreePos(Context &, const Piece piece);
        void replaceWithNewPiece(Context &, const Piece piece, const BoardPos_t & pos);

        // returns m_pieceVerts index of the piece erased, otherwise m_pieceVerts.size()
        std::size_t removePiece(Context &, const BoardPos_t & pos);

        // returns the count of pieces removed
        std::size_t removeAllPieces(Context &, const Piece piece);

        sf::Vector2i move(Context &, const BoardPos_t & fromPos, const BoardPos_t & toPos);

        void update(Context &, const float elapsedSec);
        void draw(const Context & context, sf::RenderTarget &, const sf::RenderStates &) const;
        void passEventToPieces(Context &, const sf::Event & event);

        BoardPosVec_t findAllFreePositions(const Context & context) const;

        BoardPosOpt_t findFreeBoardPosRandom(const Context & context) const;

        BoardPosVec_t findFreeBoardPosAtDistance(
            const Context & context,
            const int targetDistance,
            const DistanceRule distanceRule,
            const std::size_t count) const;

        BoardPosVec_t findFreeBoardPosAroundBody(
            const Context & context,
            const int distanceFromWall,
            const int distanceFromBody,
            const std::size_t count) const;

        void colorQuad(const BoardPos_t & pos, const sf::Color & color);
        const PosEntryOpt_t entryAt(const BoardPos_t & pos) const;

        std::string entryToString(const PosEntry & entry) const;

        BoardPos_t findLastTailPiecePos() const { return m_tailPieces.back().position(); }

        void reColorTailPieces(Context & context);

        std::size_t allPiecesCount() const;

        std::vector<BoardPos_t> findPieces(const Piece piece) const;

        std::size_t countPieces(const Piece piece) const { return findPieces(piece).size(); }

        const AdjacentInfoOpt_t
            adjacentInfoOpt(const BoardPos_t & centerPos, const sf::Keyboard::Key dir) const;

        const Surroundings surroundings(const BoardPos_t & centerPos) const;

        void shrinkTail(Context & context);

      private:
        void loadMap_New(Context & context);
        void loadMap_Same(Context & context);

        PieceBase & makePiece(Context &, const Piece piece, const BoardPos_t & pos);
        std::size_t findOrMakeFreeQuadIndex();

        std::string entryInvalidDesc(const PosEntry & entry) const;

        void setupQuad(
            Context & context,
            const std::size_t quadIndex,
            const BoardPos_t & pos,
            const sf::Color & color = m_freeVertColor);

        void freeQuad(const std::size_t quadIndex);
        void colorQuad(const std::size_t quadIndex, const sf::Color & color);
        bool isQuadIndexValid(const std::size_t index) const;
        bool isQuadFree(const std::size_t index) const;

      private:
        static inline const sf::Color m_freeVertColor{ sf::Color::Transparent };
        static inline const sf::Vertex m_freeQuadVertex{ { 0.0f, 0.0f }, m_freeVertColor };

        std::map<BoardPos_t, PosEntry> m_posEntryMap;
        std::vector<sf::Vertex> m_pieceVerts;

        std::vector<HeadPiece> m_headPieces;
        std::list<TailPiece> m_tailPieces;
        std::vector<WallPiece> m_wallPieces;
        std::vector<FoodPiece> m_foodPieces;
        std::vector<ShrinkPiece> m_shrinkPieces;
        std::vector<SlowPiece> m_slowPieces;

        // clang-format off
        static inline std::array<sf::Vector2i, 9> surroundingsPositionOffsets = {
            sf::Vector2i{ -1, -1 },  sf::Vector2i{ 0, -1 },  sf::Vector2i{ 1, -1 },
            sf::Vector2i{ -1,  0 },  sf::Vector2i{ 0,  0 },  sf::Vector2i{ 1,  0  },
            sf::Vector2i{ -1,  1 },  sf::Vector2i{ 0,  1 },  sf::Vector2i{ 1,  1  },
        };
        // clang-format on
    };
} // namespace snake

#endif // SNAKE_BOARD_HPP_INCLUDED
