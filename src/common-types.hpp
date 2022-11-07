#ifndef SNAKE_COMMON_TYPES_HPP_INCLUDED
#define SNAKE_COMMON_TYPES_HPP_INCLUDED
//
// common-types.hpp
//
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Window/Keyboard.hpp>

//

namespace snake
{
    // forward declarations
    class Board;
    class Layout;
    struct Context;
    class GameConfig;

    enum class Piece;

    // custom types
    using BoardPos_t = sf::Vector2i;
    using BoardRect_t = sf::IntRect;
    using BoardPosVec_t = std::vector<BoardPos_t>;
    using BoardPosOpt_t = std::optional<BoardPos_t>;
    using VertVec_t = std::vector<sf::Vertex>;
    using DirKeyOpt_t = std::optional<sf::Keyboard::Key>;

    // these ints are the number of columns in each row from top to bottom
    using ColCountPerRowVec_t = std::vector<int>;

    using IntRectOpt_t = std::optional<sf::IntRect>;
    using IntRectVec_t = std::vector<sf::IntRect>;
    using IntRectVecVec_t = std::vector<IntRectVec_t>;

    // values

    const BoardPos_t BoardPosInvalid{ std::numeric_limits<int>::lowest(),
                                      std::numeric_limits<int>::lowest() };
} // namespace snake

#endif // SNAKE_COMMON_TYPES_HPP_INCLUDED
