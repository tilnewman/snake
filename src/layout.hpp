#ifndef SNAKE_LAYOUT_HPP_INCLUDED
#define SNAKE_LAYOUT_HPP_INCLUDED
//
// layout.hpp
//
#include "check-macros.hpp"
#include "common-types.hpp"

#include <set>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{
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

            const sf::Vector2i pos{ posOrig };

            return (
                (pos.x >= 0) && (pos.y >= 0) && (pos.x < cell_counts.x) && (pos.y < cell_counts.y));
        }

        //

        sf::Vector2i window_size{ 0, 0 };
        sf::Vector2f window_size_f{ 0.0f, 0.0f };
        sf::IntRect window_bounds{ 0, 0, 0, 0 };
        sf::FloatRect window_bounds_f{ 0.0f, 0.0f, 0.0f, 0.0f };
        //
        sf::Vector2i cell_size{ 0, 0 };
        sf::Vector2i board_size{ 0, 0 };
        sf::Vector2i cell_counts{ 0, 0 };
        int cell_count_total{ 0 };
        std::size_t cell_count_total_st{ 0 };
        sf::Vector2i top_left_pos{ 0, 0 };
        sf::IntRect board_bounds{ 0, 0, 0, 0 };
        sf::FloatRect board_bounds_f{ 0, 0, 0, 0 };

        sf::IntRect status_bounds{ 0, 0, 0, 0 };
        sf::FloatRect status_bounds_f{ 0.0f, 0.0f, 0.0f, 0.0f };

        sf::IntRect cells_rect{ 0, 0, 0, 0 };

        BoardPosVec_t default_wall_positions;

      private:
        void regionCalculations(const GameConfig & config);
        void cellCalculations(const GameConfig & config);

      private:
        std::set<BoardPos_t> all_valid_positions;
        std::vector<sf::Vertex> cell_quad_verts;
        std::vector<std::vector<sf::IntRect>> cell_bounds_lut;
    };

} // namespace snake

#endif // SNAKE_LAYOUT_HPP_INCLUDED
