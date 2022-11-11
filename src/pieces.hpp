#ifndef SNAKE_PIECES_HPP_INCLUDED
#define SNAKE_PIECES_HPP_INCLUDED
//
// pieces.hpp
//
//
#include "common-types.hpp"
#include "keys.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>

namespace snake
{
    //
    enum class Piece
    {
        Head,
        Tail,
        Food,
        Wall,
        Slow,
        Shrink
    };

    using PieceEnumOpt_t = std::optional<Piece>;

    //
    struct MoveRecord
    {
        bool ate_pickup{ false };
        bool missed_pickup{ false };
        bool wrapped_around_board{ false };
    };

    //
    struct PosInfo
    {
        PosInfo(
            const Context & context,
            const sf::Keyboard::Key direction,
            const BoardPos_t & selfPos,
            const BoardPos_t & targetPos);

        bool isOccupied() const;
        bool isOccupiedBy(const Piece potentialPiece) const;
        bool isOccupiedButNotBy(const Piece potentialPiece) const;

        sf::Keyboard::Key dir;
        BoardPos_t pos;
        PieceEnumOpt_t piece_opt;
        int dist_to_target;
    };

    //
    class PieceBase
    {
      protected:
        PieceBase() = delete;

        PieceBase(
            Context & context,
            const Piece piece,
            const BoardPos_t & pos,
            const float timeBetweenTurnsSec = -1.0f); // see comment below

      public:
        virtual ~PieceBase() = default;

        std::string toString() const;

        inline Piece piece() const { return m_piece; }

        inline const BoardPos_t position() const { return m_position; }
        void position(Context & context, const BoardPos_t & newPosition);

        inline const sf::Color & color() const { return m_color; }
        void color(Context & context, const sf::Color & newColor);

        float turnDurationSec() const { return m_turnDurationSec; }
        void turnDurationSec(const float seconds) { m_turnDurationSec = seconds; }

        virtual void update(Context &, const float elapsedSec);
        virtual void handleEvent(Context &, const sf::Event &) {}
        virtual void takeTurn(Context &) {}

      private:
        Piece m_piece;
        sf::Color m_color;
        BoardPos_t m_position;

        float m_turnDurationSec; // negative means this piece takes one turn per frame/update()
        float m_turnElapsedSec;
    };

    //

    struct WallPiece : public PieceBase
    {
        WallPiece(Context & context, const BoardPos_t & pos)
            : PieceBase(context, Piece::Wall, pos)
        {}

        virtual ~WallPiece() override = default;

        void update(Context &, const float) override {}
    };

    //

    struct FoodPiece : public PieceBase
    {
        FoodPiece(Context & context, const BoardPos_t & pos);
        virtual ~FoodPiece() override = default;

        void update(Context &, const float) override {}
    };

    //

    struct ShrinkPiece : public PieceBase
    {
        ShrinkPiece(Context & context, const BoardPos_t & pos)
            : PieceBase(context, Piece::Shrink, pos)
        {}

        virtual ~ShrinkPiece() override = default;

        void update(Context &, const float) override {}
    };

    //

    struct SlowPiece : public PieceBase
    {
        SlowPiece(Context & context, const BoardPos_t & pos)
            : PieceBase(context, Piece::Slow, pos)
        {}

        virtual ~SlowPiece() override = default;

        void update(Context &, const float) override {}
    };

    //

    struct TailPiece : public PieceBase
    {
        TailPiece(Context & context, const BoardPos_t & pos)
            : PieceBase(context, Piece::Tail, pos)
        {}

        virtual ~TailPiece() override = default;

        void update(Context &, const float) override {}

        static inline sf::Color m_colorLight{ 64, 255, 0 };

        static inline sf::Color m_colorDark{ static_cast<sf::Uint8>(m_colorLight.r / 4),
                                             static_cast<sf::Uint8>(m_colorLight.g / 4),
                                             static_cast<sf::Uint8>(m_colorLight.b / 4) };
    };

    //

    struct HeadPiece : public PieceBase
    {
        HeadPiece(Context & context, const BoardPos_t & pos);
        virtual ~HeadPiece() override = default;

        // void update(Context &, const float elapsedSec) override;
        void handleEvent(Context & context, const sf::Event & event) override;
        void takeTurn(Context & context) override;
        void resetTailGrowCounter() { m_tailGrowRemainingCount = 0; }

      private:
        void finalizeDirectionToMove_Player(const Context & context);
        void finalizeDirectionToMove_SelfTest(Context & context);
        auto move(Context & context);
        void handleTailAfterMove(Context & context);
        void handlePickup(Context &, const BoardPos_t & newPos, const Piece piece);

      protected:
        sf::Keyboard::Key m_directionPrev;
        sf::Keyboard::Key m_directionNext;
        sf::Keyboard::Key m_directionNextNext;

        std::size_t m_tailGrowRemainingCount;

        // self-test stuff
      private:
        BoardPos_t sfPickTarget(const Context & context) const;
        sf::Keyboard::Key stPickDirection(const Context & context) const;

        std::size_t m_stMovesTowardCurrentTargetCount{ 0 };
        std::size_t m_stMovesTowardCurrentTargetCountMax{ 0 };
        BoardPos_t m_stTargetPos{ BoardPosInvalid };
    };

    //

    namespace piece
    {
        inline std::string toString(const Piece piece)
        {
            switch (piece)
            {
                case Piece::Head: return "Head";
                case Piece::Tail: return "Tail";
                case Piece::Food: return "Food";
                case Piece::Wall: return "Wall";
                case Piece::Slow: return "Slow";
                case Piece::Shrink: return "Shrink";
                default: return "";
            }
        }

        inline sf::Color toColor(const Piece piece)
        {
            switch (piece)
            {
                case Piece::Head: return sf::Color::Green;
                case Piece::Tail: return TailPiece::m_colorLight;
                case Piece::Food: return sf::Color::Yellow;
                case Piece::Wall: return sf::Color(105, 70, 35);
                case Piece::Slow: return sf::Color::Magenta;
                case Piece::Shrink: return sf::Color::Red;
                default: return sf::Color::Transparent;
            }
        }
    } // namespace piece

    inline std::ostream & operator<<(std::ostream & os, const Piece piece)
    {
        os << piece::toString(piece);
        return os;
    }
} // namespace snake

#endif // SNAKE_PIECES_HPP_INCLUDED
