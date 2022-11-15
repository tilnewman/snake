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
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{

    // Parameters that CANNOT change during play, but can be customized before play starts.
    class GameConfig
    {
      public:
        GameConfig() = default;

        std::string toString() const;

        bool isTest() const { return (is_god_mode); }

        bool is_god_mode{ false };

        std::filesystem::path media_path{ std::filesystem::current_path() / "media" };

        unsigned int sf_window_style{ static_cast<unsigned>(sf::Style::Fullscreen) };

        sf::Vector2u resolution{ 0u, 0u };

        unsigned int frame_rate_limit{ 60u }; // zero means there is no limit

        sf::Color window_background_color{ sf::Color::Black };
        sf::Color board_background_color{ sf::Color::Black };
        sf::Color alt_board_background_color{ 27, 0, 13 };
        sf::Color grow_fade_text_color{ 255, 255, 200 };

        float cell_size_window_ratio{ 0.015f };
        float status_bounds_height_ratio{ 0.02f };

        float cell_anim_grow_ratio{ 1.04f };
        float cell_anim_alpha_shrink_ratio{ 0.95f };

        bool will_put_black_border_around_cells{ true };

        float initial_volume{ 50.0f };
        float eat_sfx_pitch_start{ 0.4f };
        float eat_sfx_pitch_adj{ 0.033f };

        bool will_show_fps{ false };

        std::size_t score_per_life_bonus{ 10000 };

        bool will_limit_resolution{ false };
    };

    // Parameters that change per level and define how hard it is to play the game.
    class Level
    {
      public:
        Level() = default;

        void reset();

        bool isComplete() const;
        float completedRatio() const;

        std::size_t remainingToEat() const { return (eat_count_required - eat_count_current); }

        void handlePickupFood(const Context & context);
        void handlePickupSlow(const Context & context);

        void setupForLevelNumber(
            Context & context, const std::size_t levelNumber, const bool survived);

        BoardPosVec_t makeWallPositionsForLevelNumber(Context & context);
        BoardPos_t findNextFoodPos(const Context & context) const;
        std::size_t wallObstacleCount() const;

        std::size_t number{ 0 };
        BoardPos_t start_pos{ 0, 0 };

        std::size_t eat_count_current{ 0 };
        std::size_t eat_count_required{ 0 };

        std::size_t tail_start_length{ 0 };
        std::size_t tail_grow_after_eat{ 0 };

        float sec_per_turn_slowest{ 0.0f };
        float sec_per_turn_current{ 0.0f };
        float sec_per_turn_shrink_per_eat{ 0.0f };

        BoardPosVec_t wall_positions;
    };

    // Parameters that are specific to the game currently being played.
    class GameInPlay
    {
      public:
        GameInPlay() = default;

        std::string toString() const;

        void start(Context & context);

        void setupNextLevel(Context & context, const bool survived);

        bool isGameOver() const { return m_isGameOver; }
        std::size_t lives() const { return m_lives; }
        int score() const { return m_score; }
        const Level & level() const { return m_level; }
        int calcScoreForEating(Context & context);
        void handlePickup(Context & context, const BoardPos_t & pos, const Piece piece);

        std::string statusString(const std::string & prefix) const;

      private:
        void handlePickupFood(Context & context, const BoardPos_t & pos, const Piece piece);
        void handlePickupSlow(Context & context, const BoardPos_t & pos, const Piece piece);
        void handlePickupShrink(Context & context, const BoardPos_t & pos, const Piece piece);
        void handlePickupLethal(Context & context, const BoardPos_t & pos, const Piece piece);
        int calcLevelCompleteScoreBonus() const;
        int scoreAdj(const Context & context, const int adj);

      private:
        Level m_level;
        int m_score{ 0 };
        float m_eatSfxPitch{ 1.0f };
        std::size_t m_lives{ 0 };
        bool m_isGameOver{ false };
    };
} // namespace snake

#endif // SNAKE_SETTINGS_HPP_INCLUDED
