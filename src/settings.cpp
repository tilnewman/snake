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
    void GameConfig::verifyAllMembers()
    {
        M_CHECK_SS(std::filesystem::exists(media_path), media_path);

        try
        {
            media_path = std::filesystem::canonical(media_path);
        }
        catch (const std::filesystem::filesystem_error & fsEx)
        {
            std::cerr << "Caught Filesystem Exception in GameConfig::verifyAllMembers():  what=\""
                      << fsEx.what() << "\"";

            throw;
        }

        M_CHECK_SS(std::filesystem::is_directory(media_path), media_path);
        M_CHECK_SS((cell_size_window_ratio > 0.0f), cell_size_window_ratio);
        M_CHECK_SS((!(initial_volume < 0.0f) && !(initial_volume > 100.0f)), initial_volume);
    }

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

    std::string LevelDetails::toString() const
    {
        std::ostringstream ss;

        ss << "Level\n#" << number << ": (Details)";
        ss << "\n  eat_count_current   = " << eat_count_current;
        ss << "\n  eat_count_required  = " << eat_count_required;
        ss << "\n  tail_grow_after_eat = " << tail_grow_after_eat;
        ss << "\n  sec_per_turn_current= " << sec_per_turn_current;
        ss << "\n  turns_per_sec_min   = " << sec_per_turn_fastest;
        ss << "\n  turns_per_sec_max   = " << sec_per_turn_slowest;
        ss << "\n  sec_per_turn_adj    = " << sec_per_turn_shrink_per_eat;

        return ss.str();
    }

    float LevelDetails::completedRatio() const
    {
        float ratio{ 0.0f };

        if (eat_count_required > 0)
        {
            ratio =
                (static_cast<float>(eat_count_current) / static_cast<float>(eat_count_required));
        }

        return std::clamp(ratio, 0.0f, 1.0f);
    }

    //

    void Level::reset() { m_details = LevelDetails(); }

    std::string Level::toString() const { return m_details.toString(); }

    bool Level::isComplete() const
    {
        return (
            (m_details.eat_count_required > 0) &&
            (m_details.eat_count_current >= m_details.eat_count_required));
    }

    void Level::handlePickupFood(const Context &)
    {
        if (m_details.eat_count_current >= m_details.eat_count_required)
        {
            return;
        }

        ++m_details.eat_count_current;
        m_details.sec_per_turn_current *= m_details.sec_per_turn_shrink_per_eat;

        m_details.tail_grow_after_eat =
            (1_st + (m_details.eat_count_current * m_details.number) + m_details.number +
             m_details.eat_count_current);
    }

    void Level::handlePickupSlow(const Context &)
    {
        m_details.sec_per_turn_current = m_details.sec_per_turn_slowest;
    }

    void Level::setupForLevelNumber(
        Context & context, const std::size_t levelNumberST, const bool survived)
    {
        m_details.number = levelNumberST;

        const float levelNumberF{ static_cast<float>(levelNumberST) };

        const float levelSqrtF{ std::clamp(
            std::sqrtf(levelNumberF), 1.0f, (1.0f + (levelNumberF / 2.0f))) };

        const std::size_t levelSqrtST{ static_cast<std::size_t>(levelSqrtF) };

        m_details.start_pos = { context.layout.cell_counts / 2 };

        m_details.eat_count_current = 0;
        m_details.eat_count_required = (8 + levelSqrtST);

        const std::size_t eatCountSqrtST{ static_cast<std::size_t>(
            sqrt(m_details.eat_count_current)) };

        m_details.tail_start_length = 10;

        m_details.tail_grow_after_eat =
            ((m_details.tail_start_length / 2) + m_details.eat_count_current + levelSqrtST +
             eatCountSqrtST);

        if (m_details.number <= m_details.eat_count_required)
        {
            m_details.pickups_visible_at_start_count =
                (m_details.eat_count_required - m_details.number);
        }
        else
        {
            m_details.pickups_visible_at_start_count = 0;
        }

        // start speed where it takes 6 seconds to travel the height of the board
        m_details.sec_per_turn_slowest = (6.0f / static_cast<float>(context.layout.cell_counts.y));

        // ...and end so fast that it only takes 2 seconds
        m_details.sec_per_turn_fastest = (2.0f / static_cast<float>(context.layout.cell_counts.y));

        m_details.sec_per_turn_shrink_per_eat = 0.925f;

        m_details.sec_per_turn_current = m_details.sec_per_turn_slowest;

        if (survived)
        {
            m_details.wall_positions = makeWallPositionsForLevelNumber(context);
        }
    }

    // TODO put some better random logic based on increasing level
    BoardPosVec_t Level::makeWallPositionsForLevelNumber(Context & context)
    {
        BoardPosVec_t wallPositions;
        wallPositions.reserve(1000);

        if (context.random.boolean())
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

        if (context.random.boolean())
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

        return wallPositions;
    }

    BoardPos_t Level::findNextFoodPos(const Context & context) const
    {
        return context.board.findFreeBoardPosRandom(context).value_or(BoardPosInvalid);
    }

    //

    void GameInPlay::reset(const GameConfig & config, const Layout &)
    {
        m_score = 0;
        m_isGameOver = true;
        m_eatSfxPitch = config.eat_sfx_pitch_start;
        m_level.reset();
    }

    void GameInPlay::start(Context & context)
    {
        if (!m_isGameOver)
        {
            stop(context);
        }

        reset(context.config, context.layout);

        m_isGameOver = false;

        context.audio.volume(context.config.initial_volume);

        m_level.setupForLevelNumber(context, 1, true);
        context.board.loadMap(context, true);

        m_lives = 3;
    }

    void GameInPlay::stop(Context & context)
    {
        m_isGameOver = true;
        context.cell_anims.reset();
    }

    void GameInPlay::setupNextLevel(Context & context, const bool survived)
    {
        m_eatSfxPitch = context.config.eat_sfx_pitch_start;
        context.cell_anims.reset();

        const std::size_t nextLevelNumber{ ((survived) ? (level().number + 1) : level().number) };

        m_level.setupForLevelNumber(context, nextLevelNumber, survived);
        context.board.loadMap(context, survived);
    }

    std::string GameInPlay::toString() const
    {
        std::ostringstream ss;

        ss << "GameInPlay:";
        ss << "\n  score = " << m_score;
        ss << "\n\n" << m_level.toString();

        return ss.str();
    }

    bool GameInPlay::isLevelComplete() const { return m_level.isComplete(); }

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

        const std::size_t liveBonusesBefore = (m_score / context.config.score_per_life_bonus);
        scoreAdj(context, scoreEarned);
        const std::size_t liveBonusesAfter = (m_score / context.config.score_per_life_bonus);

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
        // re-use the slow sfx intentionally
        context.audio.play("slow", m_eatSfxPitch);

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
        }

        if (context.config.is_god_mode)
        {
            std::cout << "...but god mode saves you";
            m_lives = 1;
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

        ss << prefix << ": ";

        if (m_isGameOver)
        {
            ss << "Game is over after reaching ";
        }
        else
        {
            ss << "Playing at ";
        }

        ss << "level=" << m_level.details().number;
        ss << ", with " << m_level.details().remainingToEat();
        ss << " of " << m_level.details().eat_count_required;
        ss << " left to eat (" << (m_level.details().completedRatio() * 100.0f) << "%)";
        ss << ", score=" << m_score;

        return ss.str();
    }
} // namespace snake
