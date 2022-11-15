#ifndef SNAKE_ADJACENT_HPP_INCLUDED
#define SNAKE_ADJACENT_HPP_INCLUDED
//
// adjacent.hpp
//
#include "common-types.hpp"
#include "keys.hpp"
#include "pieces.hpp"

#include <initializer_list>
#include <optional>
#include <tuple>
#include <vector>

//

namespace snake
{

    struct AdjacentInfo
    {
        Piece piece{ Piece::Wall };
        BoardPos_t pos{ BoardPosInvalid };
        sf::Keyboard::Key dir{ keys::not_a_key };
    };

    using AdjacentInfoOpt_t = std::optional<AdjacentInfo>;

    [[nodiscard]] inline bool
        operator==(const AdjacentInfo & left, const AdjacentInfo & right) noexcept
    {
        // clang-format off
        return (std::tie(
                left.piece,
                left.pos,
                left.dir)
            == std::tie(
                right.piece,
                right.pos,
                right.dir));
        // clang-format on
    }

    [[nodiscard]] inline bool
        operator!=(const AdjacentInfo & left, const AdjacentInfo & right) noexcept
    {
        return !(left == right);
    }

    //

    struct Surroundings
    {
        explicit Surroundings(const BoardPos_t & centerPos);

        PieceEnumOpt_t pieceInDirOpt(const sf::Keyboard::Key dirToFind) const;
        BoardPosOpt_t posInDirOpt(const sf::Keyboard::Key dirToFind) const;
        BoardPos_t posInDir(const sf::Keyboard::Key dirToFind) const;

        DirKeyOpt_t dirOfPieceOpt(const Piece pieceToFind) const;
        sf::Keyboard::Key dirOfPiece(const Piece pieceToFind) const;
        BoardPosOpt_t posOfPieceOpt(const Piece pieceToFind) const;
        BoardPos_t posOfPiece(const Piece pieceToFind) const;

        PieceEnumOpt_t pieceAtPosOpt(const BoardPos_t & posToFind) const;
        DirKeyOpt_t dirOfPosOpt(const BoardPos_t & posToFind) const;
        sf::Keyboard::Key dirOfPos(const BoardPos_t & posToFind) const;

        std::size_t pieceCount(const Piece pieceToFind) const;
        std::size_t piecesCount(const std::initializer_list<Piece> & list) const;

        BoardPos_t center_pos{ BoardPosInvalid };
        std::vector<AdjacentInfo> adjacents;
    };

} // namespace snake

#endif // SNAKE_ADJACENT_HPP_INCLUDED
