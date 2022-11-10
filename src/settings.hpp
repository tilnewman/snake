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
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{

    // Parameters that CANNOT change during play, but could be customized before a new game starts.
    class GameConfig
    {
      public:
        GameConfig() = default;

        void verifyAllMembers();
        std::string toString() const;

        bool isTest() const { return (is_god_mode); }

        bool is_god_mode{ false };

        std::filesystem::path media_path{ std::filesystem::current_path() / "media" };

        unsigned int sf_window_style{ static_cast<unsigned>(sf::Style::Fullscreen) };

        sf::Vector2u resolution{ 0u, 0u };

        unsigned int frame_rate_limit{ 60u }; // zero means there is no limit

        sf::Color window_background_color{ sf::Color::Black };
        sf::Color board_background_color{ sf::Color::Black };
        sf::Color alt_board_background_color{ 37, 0, 23 };
        sf::Color board_outline_color{ 230, 190, 180, 100 };

        float cell_size_window_ratio{ 0.0175f };
        float status_bounds_height_ratio{ 0.02f };

        float cell_anim_grow_ratio{ 1.04f };
        float cell_anim_alpha_shrink_ratio{ 0.95f };

        bool will_put_black_border_around_cells{ true };

        float initial_volume{ 50.0f };
        float eat_sfx_pitch_start{ 0.4f };
        float eat_sfx_pitch_adj{ 0.033f };
    };

    //
    struct LevelDetails
    {
        std::string toString() const;
        float completedRatio() const;
        std::size_t remainingToEat() const { return (eat_count_required - eat_count_current); }
        //
        std::size_t number{ 0 };
        BoardPos_t start_pos{ 0, 0 };

        std::size_t eat_count_current{ 0 };
        std::size_t eat_count_required{ 0 };

        std::size_t tail_start_length{ 0 };
        std::size_t tail_grow_after_eat{ 0 };
        std::size_t pickups_visible_at_start_count{ 0 };

        float sec_per_turn_fastest{ 0.0f };
        float sec_per_turn_slowest{ 0.0f };
        float sec_per_turn_current{ 0.0f };
        float sec_per_turn_shrink_per_eat{ 0.0f };

        BoardPosVec_t wall_positions;
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
        void handlePickupSlow(const Context & context);
        void setupForLevelNumber(Context & context, const std::size_t levelNumber);

      private:
        BoardPosVec_t makeWallPositionsForLevelNumber(Context & context);
        BoardPos_t findNextFoodPos(const Context & context) const;

      private:
        LevelDetails m_details;

        // static inline const std::size_t levels_per_inc_eat_required{ 5 };
        static inline const float sec_per_turn_shrink_per_eat_min{ 0.2f };
        static inline const float sec_per_turn_shrink_per_level{ 0.99f };
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
        void handlePickupSlow(Context & context, const BoardPos_t & pos, const Piece piece);
        void handlePickupLethal(Context & context, const BoardPos_t & pos, const Piece piece);
        int calcLevelCompleteScoreBonus() const;
        int scoreAdj(const Context & context, const int adj);

      private:
        Level m_level;
        int m_score{ 0 };
        bool m_isGameOver{ true };
        WhoIsPlaying m_whoIsPlaying{ WhoIsPlaying::Human };
        float m_eatSfxPitch{ 1.0f };

        static inline const float m_aiPlayVolume{ 5.0f };
    };
} // namespace snake

#endif // SNAKE_SETTINGS_HPP_INCLUDED
