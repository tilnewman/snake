// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// settings.cpp
//
#include "settings.hpp"

#include "board.hpp"
#include "cell-animations.hpp"
#include "check-macros.hpp"
#include "context.hpp"
#include "layout.hpp"
#include "pieces.hpp"
#include "random.hpp"
#include "sound-player.hpp"
#include "states.hpp"
#include "status-region.hpp"
#include "util.hpp"

#include <algorithm>
#include <sstream>

namespace snake
{

    std::string GameConfig::toString() const
    {
        std::ostringstream ss;

        ss << "GameConfig:";
        ss << "\n  media_path              = " << media_path;
        ss << "\n  is_god_mode             = " << std::boolalpha << is_god_mode;
        ss << "\n  resolution              = " << resolution;
        ss << "\n  frame_rate_limit        = " << frame_rate_limit;
        ss << "\n  initial_volume          = " << initial_volume;
        ss << "\n  cell_size_window_ratio  = " << cell_size_window_ratio;
        ss << "\n  stat_reg_height_ratio   = " << status_bounds_height_ratio;
        ss << "\n  is_fullscreen           = "
           << ((sf_window_style & sf::Style::Fullscreen) ? "true"
                                                         : std::to_string(sf_window_style));

        return ss.str();
    }

    //

    float Level::completedRatio() const
    {
        float ratio{ 0.0f };

        if (eat_count_required > 0)
        {
            ratio =
                (static_cast<float>(eat_count_current) / static_cast<float>(eat_count_required));
        }

        return std::clamp(ratio, 0.0f, 1.0f);
    }

    void Level::reset()
    {
        number = 0;
        start_pos = { 0, 0 };

        eat_count_current = 0;
        eat_count_required = 0;

        tail_start_length = 0;
        tail_grow_after_eat = 0;

        sec_per_turn_slowest = 0.0f;
        sec_per_turn_current = 0.0f;
        sec_per_turn_shrink_per_eat = 0.0f;

        wall_positions.clear();
        obstacle_positions.clear();
    }

    bool Level::isComplete() const { return (eat_count_current >= eat_count_required); }

    void Level::handlePickupFood(const Context &)
    {
        ++eat_count_current;
        tail_grow_after_eat += eat_count_current;
        sec_per_turn_current *= sec_per_turn_shrink_per_eat;
    }

    void Level::handlePickupSlow(const Context &) { sec_per_turn_current = sec_per_turn_slowest; }

    void Level::setup(Context & context, const std::size_t levelNumberST, const bool survived)
    {
        number = levelNumberST;
        start_pos = { context.layout.cell_counts / 2 };

        const std::size_t levelSqrtST{ static_cast<std::size_t>(std::sqrt(levelNumberST)) };

        eat_count_current = 0;
        eat_count_required = (8 + levelSqrtST);

        tail_start_length = 10;
        tail_grow_after_eat = ((tail_start_length / 2) + (levelSqrtST * 2));

        sec_per_turn_slowest = (6.0f / static_cast<float>(context.layout.cell_counts.y));
        sec_per_turn_current = sec_per_turn_slowest;
        sec_per_turn_shrink_per_eat = 0.925f;

        if (survived)
        {
            wall_positions = makeWallPositions(context);
            obstacle_positions = makeObstaclePositions(context);
        }
    }

    BoardPosVec_t Level::makeWallPositions(const Context & context) const
    {
        BoardPosVec_t wallPositions;
        wallPositions.reserve(1000);

        if ((context.game.level().number >= 5) && context.random.boolean())
        {
            std::copy(
                std::begin(context.layout.wall_positions_top),
                std::end(context.layout.wall_positions_top),
                std::back_inserter(wallPositions));

            std::copy(
                std::begin(context.layout.wall_positions_bottom),
                std::end(context.layout.wall_positions_bottom),
                std::back_inserter(wallPositions));
        }

        if ((context.game.level().number >= 10) && context.random.boolean())
        {
            std::copy(
                std::begin(context.layout.wall_positions_left),
                std::end(context.layout.wall_positions_left),
                std::back_inserter(wallPositions));

            std::copy(
                std::begin(context.layout.wall_positions_right),
                std::end(context.layout.wall_positions_right),
                std::back_inserter(wallPositions));
        }

        return wallPositions;
    }

    BoardPosVec_t Level::makeObstaclePositions(const Context & context) const
    {
        std::size_t count = number;

        if (count > context.config.obstacle_count_limit)
        {
            count = context.config.obstacle_count_limit;
        }

        BoardPosVec_t positions;
        positions.reserve(count);

        for (std::size_t i(0); i < count; ++i)
        {
            auto boardPosOpt = context.board.findFreeBoardPosRandom(context);
            if (boardPosOpt.has_value())
            {
                positions.push_back(boardPosOpt.value());
            }
        }

        return positions;
    }

    //

    void GameInPlay::start(Context & context)
    {
        m_level.setup(context, 1, true);

        m_score = 0;
        m_eatSfxPitch = context.config.eat_sfx_pitch_start;
        m_lives = 3;
        m_isGameOver = false;

        context.board.loadMap(context, true);
    }

    void GameInPlay::setupNextLevel(Context & context, const bool survived)
    {
        m_eatSfxPitch = context.config.eat_sfx_pitch_start;

        const std::size_t nextLevelNumber{ ((survived) ? (level().number + 1) : level().number) };
        m_level.setup(context, nextLevelNumber, survived);

        context.board.loadMap(context, survived);
    }

    std::string GameInPlay::toString() const
    {
        std::ostringstream ss;

        ss << "GameInPlay:";
        ss << "\n  score = " << m_score;
        ss << "\n  Level #" << m_level.number;
        ss << "\n  eat_count_current   = " << m_level.eat_count_current;
        ss << "\n  eat_count_required  = " << m_level.eat_count_required;
        ss << "\n  tail_grow_after_eat = " << m_level.tail_grow_after_eat;
        ss << "\n  sec_per_turn_current= " << m_level.sec_per_turn_current;
        ss << "\n  turns_per_sec_max   = " << m_level.sec_per_turn_slowest;
        ss << "\n  sec_per_turn_adj    = " << m_level.sec_per_turn_shrink_per_eat;

        return ss.str();
    }

    int GameInPlay::calcScoreForEating(Context &)
    {
        const std::size_t score{ (level().number + level().eat_count_current) * 10_st };
        return static_cast<int>(score);
    }

    int GameInPlay::calcLevelCompleteScoreBonus() const
    {
        const std::size_t score{ level().number * 100_st };
        return static_cast<int>(score);
    }

    void GameInPlay::handlePickup(Context & context, const BoardPos_t & pos, const Piece piece)
    {
        if (piece == Piece::Food)
        {
            handlePickupFood(context, pos, piece);
        }
        else if (piece == Piece::Slow)
        {
            handlePickupSlow(context, pos, piece);
        }
        else if (piece == Piece::Shrink)
        {
            handlePickupShrink(context, pos, piece);
        }
        else // all other pieces (wall, head, tail) are deadly
        {
            handlePickupLethal(context, pos, piece);
        }

        context.status.updateText(context);
    }

    void GameInPlay::handlePickupFood(Context & context, const BoardPos_t & pos, const Piece piece)
    {
        context.audio.play("shine", m_eatSfxPitch);
        m_eatSfxPitch += context.config.eat_sfx_pitch_adj;

        context.cell_anims.addGrowFadeAnim(context.layout.cellBounds(pos), piece::toColor(piece));

        m_level.handlePickupFood(context);

        int scoreEarned = calcScoreForEating(context);

        if (m_level.isComplete())
        {
            scoreEarned += calcLevelCompleteScoreBonus();
            context.state.setChangePending(State::LevelCompleteMsg);
        }

        const int liveBonusesBefore =
            (m_score / static_cast<int>(context.config.score_per_life_bonus));

        scoreAdj(context, scoreEarned);

        const int liveBonusesAfter =
            (m_score / static_cast<int>(context.config.score_per_life_bonus));

        if (liveBonusesBefore == liveBonusesAfter)
        {
            context.cell_anims.addRisingText(
                context,
                "+" + std::to_string(scoreEarned),
                context.config.grow_fade_text_color,
                context.layout.cellBounds(pos));
        }
        else
        {
            ++m_lives;

            context.cell_anims.addRisingText(
                context,
                "LIFE BONUS!",
                context.config.grow_fade_text_color,
                context.layout.cellBounds(pos));
        }
    }

    void GameInPlay::handlePickupSlow(Context & context, const BoardPos_t & pos, const Piece piece)
    {
        context.audio.play("slow", m_eatSfxPitch);

        context.cell_anims.addGrowFadeAnim(context.layout.cellBounds(pos), piece::toColor(piece));

        context.cell_anims.addRisingText(
            context, "SLOW!", context.config.grow_fade_text_color, context.layout.cellBounds(pos));

        m_level.handlePickupSlow(context);
    }

    void
        GameInPlay::handlePickupShrink(Context & context, const BoardPos_t & pos, const Piece piece)
    {
        context.audio.play("explode-puff", m_eatSfxPitch);

        context.cell_anims.addGrowFadeAnim(context.layout.cellBounds(pos), piece::toColor(piece));

        context.cell_anims.addRisingText(
            context,
            "SHRINK!",
            context.config.grow_fade_text_color,
            context.layout.cellBounds(pos));

        context.board.shrinkTail(context);
    }

    void GameInPlay::handlePickupLethal(Context & context, const BoardPos_t &, const Piece piece)
    {
        if (piece == Piece::Wall)
        {
            context.audio.play("mario-break-block");
        }
        else if ((piece == Piece::Tail) || (piece == Piece::Head))
        {
            context.audio.play("step-smash-yuck");
        }

        std::cout << "Player bit into " << piece << " and loses a life with ";

        M_CHECK_SS((m_lives > 0), "GameInPlay::m_lives was zero when it should not be!");

        --m_lives;
        std::cout << m_lives << " remaining";

        if (0 == m_lives)
        {
            std::cout << " and dies";

            if (context.config.is_god_mode)
            {
                std::cout << "...but god mode saves you";
                m_lives = 1;
            }
            else
            {
                m_isGameOver = true;
            }
        }

        std::cout << ".\n";

        context.state.setChangePending(State::Over);
    }

    int GameInPlay::scoreAdj(const Context &, const int adj)
    {
        m_score += adj;

        if (m_score < 0)
        {
            m_score = 0;
        }

        return score();
    }

    std::string GameInPlay::statusString(const std::string & prefix) const
    {
        std::ostringstream ss;

        ss << prefix << ": Playing at ";
        ss << "level=" << m_level.number;
        ss << ", with " << m_level.remainingToEat();
        ss << " of " << m_level.eat_count_required;
        ss << " left to eat (" << (m_level.completedRatio() * 100.0f) << "%)";
        ss << ", score=" << m_score;

        return ss.str();
    }
} // namespace snake
