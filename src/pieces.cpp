// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// meth-head.cpp
//
#include "pieces.hpp"

#include "board.hpp"
#include "cell-animations.hpp"
#include "context.hpp"
#include "layout.hpp"
#include "random.hpp"
#include "settings.hpp"
#include "sound-player.hpp"
#include "states.hpp"
#include "util.hpp"

namespace snake
{
    PieceBase::PieceBase(
        Context &, const Piece piece, const BoardPos_t & pos, const float timeBetweenTurnsSec)
        : m_piece(piece)
        , m_color(piece::toColor(piece))
        , m_position(pos)
        , m_turnDurationSec(timeBetweenTurnsSec)
        , m_turnElapsedSec(0.0f)
    {}

    std::string PieceBase::toString() const
    {
        std::ostringstream ss;

        ss << piece::toString(m_piece) << " Piece:";
        ss << "\n\t position            = " << position();
        ss << "\n\t color               = " << color();
        ss << "\n\t turn_duration_sec   = " << m_turnDurationSec;
        ss << "\n\t turn_elapsed_sec    = " << m_turnElapsedSec;

        return ss.str();
    }

    void PieceBase::position(Context & context, const BoardPos_t & newPosition)
    {
        m_position = context.board.move(context, position(), newPosition);
    }

    void PieceBase::color(Context & context, const sf::Color & newColor)
    {
        context.board.colorQuad(position(), newColor);
        m_color = newColor;
    }

    void PieceBase::update(Context & context, const float elapsedSec)
    {
        if (m_turnDurationSec < 0.0f)
        {
            takeTurn(context);
        }
        else
        {
            m_turnElapsedSec += elapsedSec;
            while (m_turnElapsedSec > m_turnDurationSec)
            {
                takeTurn(context);
                m_turnElapsedSec -= m_turnDurationSec;
            }
        }
    }

    //

    FoodPiece::FoodPiece(Context & context, const BoardPos_t & pos)
        : PieceBase(context, Piece::Food, pos, -1.0f)
    {}

    //

    HeadPiece::HeadPiece(Context & context, const BoardPos_t & pos)
        : PieceBase(context, Piece::Head, pos, context.game.level().sec_per_turn_current)
        , m_directionPrev(keys::not_a_key)
        , m_directionNext(keys::not_a_key)
        , m_directionNextNext(keys::not_a_key)
        , m_tailGrowRemainingCount(context.game.level().tail_start_length)
    {
        const sf::Keyboard::Key initialRandomDirection{ context.random.from(
            { sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right }) };

        m_directionPrev = initialRandomDirection;
        m_directionNext = initialRandomDirection;
        m_directionNextNext = keys::not_a_key;
    }

    void HeadPiece::handleEvent(Context & context, const sf::Event & event)
    {
        if (sf::Event::KeyPressed != event.type)
        {
            return;
        }

        const auto key{ event.key.code };
        if (!keys::isArrow(key))
        {
            return;
        }

        if (keys::not_a_key == m_directionNext)
        {
            if (keys::isLateral(key, m_directionPrev))
            {
                m_directionNext = key;
                m_directionNextNext = keys::not_a_key;
                context.audio.play("tap-1-a.ogg");
            }

            return;
        }

        if (keys::not_a_key == m_directionNextNext)
        {
            if (keys::isLateral(key, m_directionNext))
            {
                m_directionNextNext = key;
                context.audio.play("tap-1-a.ogg");
            }

            return;
        }
    }

    auto HeadPiece::move(Context & context)
    {
        const BoardPos_t oldPos{ position() };
        const BoardPos_t newPos{ keys::move(oldPos, m_directionNext) };
        M_CHECK_SS((newPos != oldPos), "oldPos=" << oldPos << ", newPos=" << newPos);

        const PieceEnumOpt_t newPosEnumOpt{ context.board.pieceEnumOptAt(newPos) };

        // check for miss must occur here before things move around
        if (!newPosEnumOpt)
        {
            const Surroundings oldSurr{ context.board.surroundings(oldPos) };
            const Surroundings newSurr{ context.board.surroundings(newPos) };

            if ((oldSurr.pieceCount(Piece::Food) > 0) && (newSurr.pieceCount(Piece::Food) == 0))
            {
                context.audio.play("miss");

                context.cell_anims.addRisingText(
                    context,
                    "miss",
                    sf::Color::White,
                    context.layout.cellBounds(oldSurr.posOfPiece(Piece::Food)));
            }
        }

        position(context, newPos);
        return std::make_tuple(oldPos, newPos, newPosEnumOpt);
    }

    void HeadPiece::handlePickup(Context & context, const BoardPos_t & posEaten, const Piece piece)
    {
        context.game.handlePickup(context, posEaten, piece);

        if (context.game.isGameOver())
        {
            color(context, sf::Color::Red);
        }
        else
        {
            const LevelDetails & level{ context.game.level() };

            if (Piece::Food == piece)
            {
                m_tailGrowRemainingCount += level.tail_grow_after_eat;
            }

            turnDurationSec(level.sec_per_turn_current);
        }
    }

    void HeadPiece::handleTailAfterMove(Context & context)
    {
        if (m_tailGrowRemainingCount > 0)
        {
            --m_tailGrowRemainingCount;
        }
        else
        {
            context.board.removePiece(context, context.board.findLastTailPiecePos());
        }

        context.board.reColorTailPieces(context);
    }

    void HeadPiece::takeTurn(Context & context)
    {
        if (context.game.isGameOver())
        {
            return;
        }

        if (context.game.isHumanPlaying())
        {
            finalizeDirectionToMove_Player(context);
        }
        else
        {
            finalizeDirectionToMove_SelfTest(context);
        }

        const auto [oldPos, newPos, newPosEnumOpt] = move(context);

        context.board.replaceWithNewPiece(context, Piece::Tail, oldPos);

        // handlePickup() must occur before handleTailAfterMove() to keep
        // m_tailGrowRemainingCount in sync
        if (newPosEnumOpt)
        {
            handlePickup(context, newPos, newPosEnumOpt.value());
        }

        handleTailAfterMove(context);

        m_directionPrev = m_directionNext;
        m_directionNext = m_directionNextNext;
        m_directionNextNext = keys::not_a_key;
    }

    //

    void HeadPiece::finalizeDirectionToMove_SelfTest(Context & context)
    {
        M_CHECK_SS(keys::isArrow(m_directionPrev), m_directionPrev);

        ++m_stMovesTowardCurrentTargetCount;

        const bool haveTarget{ context.layout.isPositionValid(m_stTargetPos) };

        const bool isThereStillFoodAtTarget{
            haveTarget &&
            (context.board.pieceEnumOptAt(m_stTargetPos).value_or(Piece::Wall) == Piece::Food)
        };

        const bool isTimeToGiveUpOnRechingTarget{ m_stMovesTowardCurrentTargetCount >
                                                  m_stMovesTowardCurrentTargetCountMax };

        const bool willPickNewTarget{ !haveTarget || !isThereStillFoodAtTarget ||
                                      isTimeToGiveUpOnRechingTarget };

        if (willPickNewTarget)
        {
            // clang-format off
           // std::cout << " *** picking new target:";
           // std::cout << "\n\t haveTarget                    = " << std::boolalpha << haveTarget;
           // //std::cout << "\n\t isThereStillFoodAtTarget      = " << std::boolalpha << isThereStillFoodAtTarget;
           // std::cout << "\n\t isTimeToGiveUpOnRechingTarget = " << std::boolalpha << isTimeToGiveUpOnRechingTarget;
           // std::cout << "\n\t m_directionNextBEFORE         = " << m_directionNext;
           // std::cout << "\n\t m_stMovesTowardCurrent        = " << m_stMovesTowardCurrentTargetCount;
           // std::cout << "\n\t m_stTargetPosBEFORE           = " << m_stTargetPos;
            // clang-format on

            m_stMovesTowardCurrentTargetCount = 0;

            m_stTargetPos = sfPickTarget(context);
            // std::cout << "\n\t m_stTargetPosAFTER            = " << m_stTargetPos;

            // Need a number of moves toward a target that is considered too much,
            // and assume we will never reach the current target and pick a new on.
            // A lot of different values would easily work here, but basing it on
            // the number of cells/board-size makes the most sense.
            const std::size_t movesMax{
                20_st +
                (static_cast<std::size_t>(std::sqrt(context.layout.cell_count_total)) * 2_st)
            };

            const std::size_t movesMin{ movesMax / 2 };

            // std::cout << "\n\t m_stMovesTowardMaxBEFORE      = "
            //           << m_stMovesTowardCurrentTargetCountMax;

            m_stMovesTowardCurrentTargetCountMax = context.random.fromTo(movesMin, movesMax);

            // std::cout << "\n\t m_stMovesTowardMaxAFTER       = "
            //          << m_stMovesTowardCurrentTargetCountMax << ":  [" << movesMin << ","
            //          << movesMax << "]" << std::endl;
        }

        m_directionNext = stPickDirection(context);

        if (!keys::isArrow(m_directionNext))
        {
            // std::cout << " *** finalizeDirectionToMove_SelfTest forced to use prev" << std::endl;
            m_directionNext = m_directionPrev;
            m_directionNextNext = keys::not_a_key;
        }
    }

    void HeadPiece::finalizeDirectionToMove_Player(const Context &)
    {
        M_CHECK_SS(keys::isArrow(m_directionPrev), m_directionPrev);

        if (m_directionNext == m_directionNextNext)
        {
            M_CHECK_SS(
                (m_directionNext == keys::not_a_key),
                "(1)m_directionNext=" << m_directionNext
                                      << ", m_directionNextNext=" << m_directionNextNext);
        }

        //  reversing direction leading to instant death should be prevented elsewhere
        M_CHECK_SS(
            (keys::opposite(m_directionNext) != m_directionPrev),
            "(1)Reverse direction move detected: m_directionPrev="
                << m_directionPrev << ", m_directionNext=" << m_directionNext
                << ", m_directionNextNext=" << m_directionNext);

        if (!keys::isArrow(m_directionNext))
        {
            m_directionNext = m_directionPrev;
            m_directionNextNext = keys::not_a_key;
        }

        if (m_directionNext == m_directionNextNext)
        {
            M_CHECK_SS(
                (m_directionNext == keys::not_a_key),
                "(2)m_directionNext=" << m_directionNext
                                      << ", m_directionNextNext=" << m_directionNextNext);
        }

        //  reversing direction leading to instant death should be prevented elsewhere
        M_CHECK_SS(
            (keys::opposite(m_directionNext) != m_directionPrev),
            "(2)Reverse direction move detected: m_directionPrev="
                << m_directionPrev << ", m_directionNext=" << m_directionNext
                << ", m_directionNextNext=" << m_directionNext);
    }

    //

    BoardPos_t HeadPiece::sfPickTarget(const Context & context) const
    {
        static std::vector<std::pair<int, BoardPos_t>> targets;
        targets.reserve(1000);
        targets.clear();

        const sf::Vector2i posSelf{ position() };

        for (const BoardPos_t & foodPos : context.board.findPieces(Piece::Food))
        {
            const sf::Vector2i posFood{ foodPos };
            const sf::Vector2i posDiff{ posFood - posSelf };
            const int distance{ std::abs(posDiff.x) + std::abs(posDiff.y) };
            targets.push_back({ distance, foodPos });
        }

        if (targets.empty())
        {
            // std::cout << " *** sfPickTarget() found no food" << std::endl;
            return BoardPosInvalid;
        }
        else if (targets.size() == 1)
        {
            return targets.front().second;
        }

        std::sort(std::begin(targets), std::end(targets));
        return targets.front().second;
    }

    sf::Keyboard::Key HeadPiece::stPickDirection(const Context & context) const
    {
        // sometimes it just makes mistakes...
        if (context.random.ratio() > 0.999f)
        {
            return context.random.from({
                sf::Keyboard::Up,
                sf::Keyboard::Down,
                sf::Keyboard::Left,
                sf::Keyboard::Right,
            });
        }

        static std::vector<PosInfo> posInfos;

        posInfos.clear();
        posInfos.push_back({ context, sf::Keyboard::Up, position(), m_stTargetPos });
        posInfos.push_back({ context, sf::Keyboard::Down, position(), m_stTargetPos });
        posInfos.push_back({ context, sf::Keyboard::Left, position(), m_stTargetPos });
        posInfos.push_back({ context, sf::Keyboard::Right, position(), m_stTargetPos });

        posInfos.erase(
            std::remove_if(
                std::begin(posInfos),
                std::end(posInfos),
                [&](const PosInfo & info) { return info.isOccupiedButNotBy(Piece::Food); }),
            std::end(posInfos));

        // if no valid moves then don't change direction
        if (posInfos.empty())
        {
            // std::cout << " *** stPickDirection() found no moves that were free or food."
            //          << std::endl;

            return m_directionNext;
        }
        else if (posInfos.size() == 1)
        {
            return posInfos.front().dir;
        }

        // sometimes it just makes mistakes...
        if (context.random.ratio() > 0.999f)
        {
            return context.random.from(posInfos).dir;
        }

        // for (const auto & info : posInfos)
        //{
        //    if (info.isOccupiedBy(Piece::Food))
        //    {
        //        return info.dir;
        //    }
        //}

        std::sort(
            std::begin(posInfos), std::end(posInfos), [&](const PosInfo & A, const PosInfo & B) {
                if (A.isOccupiedBy(Piece::Food) != B.isOccupiedBy(Piece::Food))
                {
                    return A.isOccupiedBy(Piece::Food);
                }
                else
                {
                    return (A.dist_to_target < B.dist_to_target);
                }
            });

        // if ((posInfos[1].dist_to_target - posInfos[0].dist_to_target) < 2.0f)
        //{
        //    return m_directionPrev;
        //}

        return posInfos.front().dir;
    }

    //

    PosInfo::PosInfo(
        const Context & context,
        const sf::Keyboard::Key direction,
        const BoardPos_t & selfPos,
        const BoardPos_t & targetPos)
        : dir(direction)
        , pos(keys::move(selfPos, dir))
        , piece_opt(context.board.pieceEnumOptAt(pos))
        , dist_to_target(0)
    {
        const sf::Vector2i posDiff{ targetPos - pos };
        dist_to_target = (std::abs(posDiff.x) + std::abs(posDiff.y));
    }

    bool PosInfo::isOccupied() const { return piece_opt.has_value(); }

    bool PosInfo::isOccupiedBy(const Piece potentialPiece) const
    {
        return (isOccupied() && (piece_opt.value() == potentialPiece));
    }

    bool PosInfo::isOccupiedButNotBy(const Piece potentialPiece) const
    {
        return (isOccupied() && (piece_opt.value() != potentialPiece));
    }
} // namespace snake
