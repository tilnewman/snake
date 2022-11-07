#ifndef SNAKE_SETTINGS_HPP_INCLUDED
#define SNAKE_SETTINGS_HPP_INCLUDED
//
// settings.hpp
//
#include "board.hpp"
#include "check-macros.hpp"
#include "common-types.hpp"
#include "util.hpp"

#include <filesystem>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{
    struct TeleportWallPos
    {
        BoardPos_t pos { 0, 0 };
        int count { 0 };
    };

    using TeleportWallPosVec_t = std::vector<TeleportWallPos>;

    //

    // Parameters that define how the board is divided into cells (square tiles).
    class Layout
    {
    public:
        Layout() = default;

        void reset(const GameConfig & config);

        std::string toString() const;

        sf::FloatRect cellBounds(const BoardPos_t & pos) const;
        const std::set<BoardPos_t> & allValidPositions() const { return all_valid_positions; }
        const std::vector<sf::Vertex> & cellVerts() const { return cell_quad_verts; }
        BoardPosOpt_t findWraparoundPos(const BoardPos_t & pos) const;

        template <typename T>
        bool isPositionValid(const sf::Vector2<T> & posOrig) const
        {
            static_assert(std::is_arithmetic_v<T>);
            static_assert(!std::is_same_v<std::remove_cv_t<T>, bool>);

            const sf::Vector2i pos { posOrig };

            return (
                (pos.x >= 0) && (pos.y >= 0) && (pos.x < cell_counts.x) && (pos.y < cell_counts.y));
        }

        //

        sf::Vector2i window_size { 0, 0 };
        sf::Vector2f window_size_f { 0.0f, 0.0f };
        sf::IntRect window_bounds { 0, 0, 0, 0 };
        sf::FloatRect window_bounds_f { 0.0f, 0.0f, 0.0f, 0.0f };
        //
        sf::Vector2i cell_size { 0, 0 };
        sf::Vector2i board_size { 0, 0 };
        sf::Vector2i cell_counts { 0, 0 };
        int cell_count_total { 0 };
        std::size_t cell_count_total_st { 0 };
        sf::Vector2i top_left_pos { 0, 0 };
        sf::IntRect board_bounds { 0, 0, 0, 0 };
        sf::FloatRect board_bounds_f { 0, 0, 0, 0 };

        sf::IntRect status_bounds { 0, 0, 0, 0 };
        sf::FloatRect status_bounds_f { 0.0f, 0.0f, 0.0f, 0.0f };

        sf::IntRect cells_rect { 0, 0, 0, 0 };

        BoardPosVec_t default_wall_positions;

    private:
        void regionCalculations(const GameConfig & config);
        void cellCalculations(const GameConfig & config);

    private:
        std::set<BoardPos_t> all_valid_positions;
        std::vector<sf::Vertex> cell_quad_verts;
        std::vector<std::vector<sf::IntRect>> cell_bounds_lut;
    };

    //

    // Parameters that CANNOT change during play, but could be customized before a new game starts.
    class GameConfig
    {
    public:
        GameConfig() = default;

        // no reset() function because these are the values that are never supposed to change
        // void reset();
        void verifyAllMembers();
        std::string toString() const;

        bool isTest() const
        {
            return (
                is_god_mode || is_speed_test || is_level_test || is_level_test_manual
                || is_all_video_mode_test);
        }

        bool is_god_mode { false };
        bool is_speed_test { false };
        bool is_level_test { false };
        bool is_level_test_manual { false };
        bool is_all_video_mode_test { false };

        std::filesystem::path media_path { std::filesystem::current_path() / "media" };

        unsigned int sf_window_style { static_cast<unsigned>(sf::Style::Fullscreen) };

        sf::Vector2u resolution { sf::VideoMode::getDesktopMode().width,
                                  sf::VideoMode::getDesktopMode().height };

        unsigned int frame_rate_limit { 60u }; // zero means there is no limit

        sf::Color window_background_color { sf::Color::Black };
        sf::Color board_background_color { 25, 0, 5 };
        sf::Color alt_board_background_color { 37, 0, 23 };
        sf::Color board_outline_color { 230, 190, 180, 100 };

        float cell_size_window_ratio { 0.0175f };
        // float board_size_reduction_ratio{ 1.0f }; // { 0.9f };
        float status_bounds_height_ratio { 0.02f };

        float cell_anim_grow_ratio { 1.04f };
        float cell_anim_alpha_shrink_ratio { 0.95f };

        bool will_put_black_border_around_cells { true };

        float initial_volume { 50.0f };
        float eat_sfx_pitch_start { 0.4f };
        float eat_sfx_pitch_adj { 0.033f };
    };

    //
    struct LevelDetails
    {
        std::string toString() const;
        float completedRatio() const;
        std::size_t remainingToEat() const { return (eat_count_required - eat_count_current); }
        //
        std::size_t number { 0 };
        BoardPos_t start_pos { 0, 0 };

        std::size_t eat_count_current { 0 };
        std::size_t eat_count_required { 0 };

        std::size_t tail_start_length { 0 };
        std::size_t tail_grow_after_eat { 0 };
        std::size_t pickups_visible_at_start_count { 0 };

        float sec_per_turn_fastest { 0.0f };
        float sec_per_turn_slowest { 0.0f };
        float sec_per_turn_current { 0.0f };
        float sec_per_turn_shrink_per_eat { 0.0f };

        // tile::Job tiles;
        BoardPosVec_t wall_positions;
        TeleportWallPosVec_t teleport_positions;
    };

    // Parameters that change per level and define how hard it is to play the game.
    class Level
    {
    public:
        Level() = default;

        // set all default values, but IS NOT READY TO PLAY
        void reset();

        std::string toString() const;

        bool isComplete() const;
        const LevelDetails & details() const { return m_details; }
        void handlePickupFood(const Context & context);
        void setupForLevelNumber(Context & context, const std::size_t levelNumber);

        // static void testLevelSetupsSkippingRepeats(Context & context);

    private:
        BoardPosVec_t makeWallPositionsForLevelNumber(Context & context);
        TeleportWallPosVec_t makeTeleportPositionsForLevelNumber(Context & context) const;
        BoardPos_t findNextFoodPos(const Context & context) const;

    private:
        LevelDetails m_details;

        // static inline const std::size_t levels_per_inc_eat_required{ 5 };
        static inline const float sec_per_turn_shrink_per_eat_min { 0.2f };
        static inline const float sec_per_turn_shrink_per_level { 0.99f };
    };

    //

    enum class WhoIsPlaying
    {
        Human,
        Ai
    };

    // Parameters that are specific to a game currently being played.
    class GameInPlay
    {
    public:
        GameInPlay() = default;

        std::string toString() const;

        // set all default values, but IS NOT READY TO PLAY
        void reset(const GameConfig & config, const Layout & layout);

        void start(Context & context, const WhoIsPlaying whoIsPlaying);
        void stop(Context &);

        void setupNextLevel(Context & context, const bool survived);

        bool isGameOver() const { return m_isGameOver; }
        int score() const { return m_score; }
        bool isHumanPlaying() const { return (WhoIsPlaying::Human == m_whoIsPlaying); }
        const LevelDetails & level() const { return m_level.details(); }
        bool isLevelComplete() const;
        int calcScoreForEating(Context & context);
        void handlePickup(Context & context, const BoardPos_t & pos, const Piece piece);

        std::string statusString(const std::string & prefix) const;

    private:
        void handlePickupFood(Context & context, const BoardPos_t & pos, const Piece piece);
        void handlePickupLethal(Context & context, const BoardPos_t & pos, const Piece piece);
        int calcLevelCompleteScoreBonus() const;
        int scoreAdj(const Context & context, const int adj);

    private:
        Level m_level;
        int m_score { 0 };
        bool m_isGameOver { true };
        WhoIsPlaying m_whoIsPlaying { WhoIsPlaying::Human };
        float m_eatSfxPitch { 1.0f };

        static inline const float m_aiPlayVolume { 5.0f };
    };
} // namespace snake

#endif // SNAKE_SETTINGS_HPP_INCLUDED
