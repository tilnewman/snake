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
    void Layout::reset(const GameConfig & config)
    {
        window_size = { 0, 0 };
        window_size_f = { 0.0f, 0.0f };
        window_bounds = { 0, 0, 0, 0 };
        window_bounds_f = { 0.0f, 0.0f, 0.0f, 0.0f };

        cell_size = { 0, 0 };
        board_size = { 0, 0 };
        cell_counts = { 0, 0 };
        cell_count_total = { 0 };
        cell_count_total_st = { 0 };
        top_left_pos = { 0, 0 };
        board_bounds = { 0, 0, 0, 0 };
        board_bounds_f = { 0, 0, 0, 0 };

        status_bounds = { 0, 0, 0, 0 };
        status_bounds_f = { 0.0f, 0.0f, 0.0f, 0.0f };

        cells_rect = { 0, 0, 0, 0 };

        all_valid_positions.clear();
        cell_quad_verts.clear();
        cell_bounds_lut.clear();

        regionCalculations(config);
        cellCalculations(config);
    }

    void Layout::regionCalculations(const GameConfig & config)
    {
        window_size = sf::Vector2i{ config.resolution };
        window_size_f = sf::Vector2f{ window_size };

        window_bounds = sf::IntRect({ 0, 0 }, window_size);
        window_bounds_f = sf::FloatRect(window_bounds);

        const float windowSizeAvg{ (window_size_f.x + window_size_f.y) / 2.0f };
        const float cellSideLengthEst{ windowSizeAvg * config.cell_size_window_ratio };
        const int cellSideLength{ util::makeEvenCopy(static_cast<int>(cellSideLengthEst), true) };

        M_CHECK_SS((cellSideLength > 0), cellSideLength);

        cell_size.x = cellSideLength;
        cell_size.y = cellSideLength;

        const int statusRegionHeightEstimate{ util::makeEvenCopy(
            static_cast<int>(window_size_f.y * config.status_bounds_height_ratio), false) };

        const sf::IntRect boardFence{ 0,
                                      0,
                                      window_size.x,
                                      util::makeEvenCopy(
                                          (window_size.y - statusRegionHeightEstimate), true) };

        M_CHECK_SS(
            ((boardFence.left >= 0) && (boardFence.top >= 0) && (boardFence.width > 0) &&
             (boardFence.height > 0)),
            boardFence);

        cell_counts = (util::size(boardFence) / cell_size);

        // Subtract ONE from each dimmension to make sure there is a nice border between the edges
        // of the board and the edges of the window.
        cell_counts.y -= 1;

        cell_count_total = (cell_counts.x * cell_counts.y);
        cell_count_total_st = static_cast<std::size_t>(cell_count_total);
        M_CHECK_SS((cell_count_total_st > 0), cell_count_total_st);

        cells_rect = sf::IntRect({ 0, 0 }, cell_counts);

        board_size = (cell_counts * cell_size);

        M_CHECK_SS(
            ((board_size.x <= window_size.x) && (board_size.y <= window_size.y)),
            "board_size=" << board_size << ", windowSizeInt=" << window_size);

        M_CHECK_SS(
            ((board_size.x <= boardFence.width) && (board_size.y <= boardFence.height)),
            "board_size=" << board_size << ", boardFence=" << boardFence);

        top_left_pos = ((util::size(boardFence) - board_size) / 2);
        top_left_pos.y = top_left_pos.x;

        board_bounds = sf::IntRect(top_left_pos, board_size);
        board_bounds_f = sf::FloatRect{ board_bounds };

        M_CHECK_SS(
            ((board_bounds.left >= 0) && (board_bounds.top >= 0) && (board_bounds.width > 0) &&
             (board_bounds.height > 0)),
            board_bounds);

        status_bounds.left = 0;
        status_bounds.width = window_size.x;
        status_bounds.top = util::bottom(board_bounds) + cell_size.y + 1;
        status_bounds.height = (window_size.y - status_bounds.top - 1);

        status_bounds_f = sf::FloatRect(status_bounds);

        // ...and now flip it so the status region is on top...
        status_bounds.top = 0;
        status_bounds_f = sf::FloatRect(status_bounds);

        const int newBoardTop{ (window_size.y - board_size.y) - board_bounds.left };
        top_left_pos.y = newBoardTop;
        board_bounds.top = newBoardTop;
        board_bounds_f = sf::FloatRect{ board_bounds };
        status_bounds.height = (board_bounds.top - 1);
        status_bounds_f = sf::FloatRect(status_bounds);

        // std::cout << "\n  window_bounds    = " << window_bounds << '/' << window_bounds_f;
        // std::cout << "\n  status_bounds    = " << status_bounds << '/'
        //          << status_bounds_f;
        // std::cout << "\n  board_bounds     = " << board_bounds << '/' << board_bounds_f;
        // std::cout << "\n  vert_sum         = "
        //          << (status_bounds.height + board_bounds.height);
        // std::cout << std::endl << std::endl;
    }

    void Layout::cellCalculations(const GameConfig & config)
    {
        // clear/reserve/resize all containers
        cell_quad_verts.clear();
        cell_quad_verts.reserve(cell_count_total_st);

        all_valid_positions.clear();

        cell_bounds_lut.clear();

        cell_bounds_lut.resize(
            static_cast<std::size_t>(cell_counts.y),
            IntRectVec_t(static_cast<std::size_t>(cell_counts.x), sf::IntRect{}));

        for (int vert(0); vert < cell_counts.y; ++vert)
        {
            for (int horiz(0); horiz < cell_counts.x; ++horiz)
            {
                const BoardPos_t pos{ horiz, vert };

                sf::IntRect bounds{ (top_left_pos + (pos * cell_size)), cell_size };

                //
                bool willShadeThisCell{ (horiz % 2) == 0 };
                if ((vert % 2) == 0)
                {
                    willShadeThisCell = !willShadeThisCell;
                }

                if (willShadeThisCell)
                {
                    util::appendQuadVerts(
                        sf::FloatRect(bounds), cell_quad_verts, config.alt_board_background_color);
                }

                //
                if (config.will_put_black_border_around_cells)
                {
                    ++bounds.left;
                    ++bounds.top;
                    bounds.width -= 2;
                    bounds.height -= 2;
                }

                M_CHECK_SS(
                    ((bounds.left >= 0) && (bounds.top >= 0) && (bounds.width > 1) &&
                     (bounds.height > 1)),
                    "board_pos=" << pos << ", cell_bounds=" << bounds);

                //
                all_valid_positions.insert(pos);

                //
                const sf::Vector2s posST(pos);
                cell_bounds_lut[posST.y][posST.x] = bounds;

                //
                if ((pos.x == 0) || (pos.y == 0) || (pos.x == (cell_counts.x - 1)) ||
                    (pos.y == (cell_counts.y - 1)))
                {
                    default_wall_positions.push_back(pos);
                }
            }
        }

        std::sort(std::begin(default_wall_positions), std::end(default_wall_positions));

        M_CHECK_SS(
            (all_valid_positions.size() == cell_count_total_st),
            "all_valid_positions.size()=" << all_valid_positions.size()
                                          << ", cell_count_total=" << cell_count_total_st);
    }

    std::string Layout::toString() const
    {
        std::ostringstream ss;

        ss << "Layout:";
        ss << "\n  window_bounds    = " << window_bounds << '/' << window_bounds_f;
        ss << "\n  board_bounds     = " << board_bounds << '/' << board_bounds_f;
        ss << "\n  status_bounds    = " << status_bounds << '/' << status_bounds_f;
        ss << "\n  top_left_pos     = " << top_left_pos;
        ss << "\n  cell_size        = " << cell_size;
        ss << "\n  cell_counts      = " << cell_counts;
        ss << "\n  cell_count_total = " << cell_count_total;

        return ss.str();
    }

    sf::FloatRect Layout::cellBounds(const BoardPos_t & pos) const
    {
        if (!isPositionValid(pos))
        {
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        const sf::Vector2s posST{ pos };
        return sf::FloatRect{ cell_bounds_lut[posST.y][posST.x] };
    }

    BoardPosOpt_t Layout::findWraparoundPos(const BoardPos_t & pos) const
    {
        if (pos.x == -1)
        {
            return BoardPos_t((cell_counts.x - 1), pos.y);
        }
        else if (pos.y == -1)
        {
            return BoardPos_t(pos.x, (cell_counts.y - 1));
        }
        else if (pos.x == cell_counts.x)
        {
            return BoardPos_t(0, pos.y);
        }
        else if (pos.y == cell_counts.y)
        {
            return BoardPos_t(pos.x, 0);
        }

        return std::nullopt;
    }

    //

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
        ss << "\n  is_speed_test           = " << std::boolalpha << is_speed_test;
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
        const LevelDetails & lvl{ m_details };

        return ((lvl.eat_count_required > 0) && (lvl.eat_count_current >= lvl.eat_count_required));
    }

    void Level::handlePickupFood(const Context &)
    {
        LevelDetails & lvl{ m_details };

        if (lvl.eat_count_current >= lvl.eat_count_required)
        {
            return;
        }

        ++lvl.eat_count_current;
        lvl.sec_per_turn_current *= lvl.sec_per_turn_shrink_per_eat;

        lvl.tail_grow_after_eat =
            (1_st + (lvl.eat_count_current * lvl.number) + lvl.number + lvl.eat_count_current);
    }

    void Level::setupForLevelNumber(Context & context, const std::size_t levelNumberST)
    {
        LevelDetails & lvl{ m_details };

        lvl.number = levelNumberST;

        const float levelNumberF{ static_cast<float>(levelNumberST) };

        const float levelSqrtF{ std::clamp(
            std::sqrtf(levelNumberF), 1.0f, (1.0f + (levelNumberF / 2.0f))) };

        const std::size_t levelSqrtST{ static_cast<std::size_t>(levelSqrtF) };

        lvl.start_pos = { context.layout.cell_counts / 2 };

        lvl.eat_count_current = 0;
        lvl.eat_count_required = (8 + lvl.number);

        const std::size_t eatCountSqrtST{ static_cast<std::size_t>(sqrt(lvl.eat_count_current)) };

        lvl.tail_start_length = 10;

        lvl.tail_grow_after_eat = (3_st + lvl.eat_count_current + levelSqrtST + eatCountSqrtST);

        if (lvl.number <= lvl.eat_count_required)
        {
            lvl.pickups_visible_at_start_count = (lvl.eat_count_required - lvl.number);
        }
        else
        {
            lvl.pickups_visible_at_start_count = 0;
        }

        // start speed where it takes 6 seconds to travel the height of the board
        lvl.sec_per_turn_slowest = (6.0f / static_cast<float>(context.layout.cell_counts.y));

        // ...and end so fast that it only takes 2 seconds
        lvl.sec_per_turn_fastest = (2.0f / static_cast<float>(context.layout.cell_counts.y));

        lvl.sec_per_turn_shrink_per_eat = 0.925f;
        //    = (1.0f
        //       - ((lvl.sec_per_turn_slowest - lvl.sec_per_turn_fastest)
        //          / static_cast<float>(lvl.eat_count_required)));

        lvl.sec_per_turn_current = lvl.sec_per_turn_slowest;

        lvl.wall_positions = makeWallPositionsForLevelNumber(context);

        // std::cout << "Setting up for level #" << levelNumberST << ":";
        // std::cout << "\n\t level_sqrt             = " << levelSqrtST;
        // std::cout << "\n\t eat_count_required     = " << lvl.eat_count_required;
        // std::cout << "\n\t tail_grow_after_eat    = " << lvl.tail_grow_after_eat;
        // std::cout << "\n\t pickups_at_start_count = " << lvl.pickups_visible_at_start_count;
        // std::cout << "\n\t sec_per_turn_slowest   = " << lvl.sec_per_turn_slowest;
        // std::cout << "\n\t sec_per_turn_fastest   = " << lvl.sec_per_turn_fastest;
        // std::cout << "\n\t sec_per_turn_current   = " << lvl.sec_per_turn_current;
        // std::cout << std::endl << std::endl;
        //
        // std::cout << toString() << std::endl;
    }

    BoardPosVec_t Level::makeWallPositionsForLevelNumber(Context & context)
    {
        return context.layout.default_wall_positions;
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
        m_whoIsPlaying = WhoIsPlaying::Human;
        m_eatSfxPitch = config.eat_sfx_pitch_start;
        m_level.reset();
    }

    void GameInPlay::start(Context & context, const WhoIsPlaying whoIsPlaying)
    {
        if (!m_isGameOver)
        {
            stop(context);
        }

        reset(context.config, context.layout);

        m_isGameOver = false;

        m_whoIsPlaying = whoIsPlaying;
        if (isHumanPlaying())
        {
            context.audio.volume(context.config.initial_volume);
        }
        else
        {
            context.audio.volume(m_aiPlayVolume);
        }

        m_level.setupForLevelNumber(context, 1);
        context.board.loadMap(context);
    }

    void GameInPlay::stop(Context & context)
    {
        m_isGameOver = true;
        context.cell_anims.reset();
    }

    void GameInPlay::setupNextLevel(Context & context, const bool survived)
    {
        // TODO Level needs start()/stop() too, where this stuff should maybe go?

        m_eatSfxPitch = context.config.eat_sfx_pitch_start;
        context.cell_anims.reset();

        const std::size_t nextLevelNumber{ ((survived) ? (level().number + 1) : level().number) };

        m_level.setupForLevelNumber(context, nextLevelNumber);
        context.board.loadMap(context);
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
        const std::size_t score{ level().number + level().eat_count_current + 1 };
        return static_cast<int>(score * 10);
    }

    int GameInPlay::calcLevelCompleteScoreBonus() const
    {
        const std::size_t score{ level().number + level().eat_count_current + 1 };
        return static_cast<int>(score * 100);
    }

    void GameInPlay::handlePickup(Context & context, const BoardPos_t & pos, const Piece piece)
    {
        if (piece == Piece::Food)
        {
            handlePickupFood(context, pos, piece);
        }
        else
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

        const int scoreEarned{ calcScoreForEating(context) };
        scoreAdj(context, scoreEarned);

        if (m_level.isComplete())
        {
            scoreAdj(context, calcLevelCompleteScoreBonus());
            context.state.setChangePending(State::LevelCompleteMsg);
        }
        else
        {
            context.cell_anims.addRisingText(
                context,
                "+" + std::to_string(scoreEarned),
                sf::Color(255, 255, 200),
                context.layout.cellBounds(pos));

            if ((level().remainingToEat() > 0) && (context.board.countPieces(Piece::Food) == 0))
            {
                context.board.addNewPieceAtRandomFreePos(context, Piece::Food);
            }
        }
    }

    void GameInPlay::handlePickupLethal(Context & context, const BoardPos_t &, const Piece piece)
    {
        if (piece == Piece::Wall)
        {
            context.audio.play("mario-break-block");
        }
        else if (piece == Piece::Tail)
        {
            context.audio.play("step-smash-yuck");
        }

        std::cout << "Player bit into " << piece << " and dies.\n";

        if (context.config.is_god_mode)
        {
            std::cout << "...but god mode saves you!\n";
            return;
        }

        // comment this out to allow players to keep retrying a failed level,
        // but this would work better if there were a life/lives counter...
        // m_isGameOver = true;

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

        ss << prefix << ":  ";
        ss << ((WhoIsPlaying::Human == m_whoIsPlaying) ? "Human" : "Ai");

        if (m_isGameOver)
        {
            ss << "'s game is over after reaching ";
        }
        else
        {
            ss << " is playing ";
        }

        auto & details{ m_level.details() };
        ss << "level=" << details.number;
        ss << ", with " << details.remainingToEat();
        ss << " of " << details.eat_count_required;
        ss << " left to eat (" << (details.completedRatio() * 100.0f) << "%)";
        ss << ", score=" << m_score;

        return ss.str();
    }
} // namespace snake
