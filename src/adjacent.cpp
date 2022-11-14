// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "adjacent.hpp"

namespace snake
{
    Surroundings::Surroundings(const BoardPos_t & centerPos)
        : center_pos(centerPos)
        , adjacents()
    {
        adjacents.reserve(4);
    }

    PieceEnumOpt_t Surroundings::pieceInDirOpt(const sf::Keyboard::Key dirToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (dirToFind == ap.dir)
            {
                return ap.piece;
            }
        }

        return std::nullopt;
    }

    BoardPosOpt_t Surroundings::posInDirOpt(const sf::Keyboard::Key dirToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (dirToFind == ap.dir)
            {
                return ap.pos;
            }
        }

        return std::nullopt;
    }

    BoardPos_t Surroundings::posInDir(const sf::Keyboard::Key dirToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (dirToFind == ap.dir)
            {
                return ap.pos;
            }
        }

        return BoardPosInvalid;
    }

    DirKeyOpt_t Surroundings::dirOfPieceOpt(const Piece pieceToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (pieceToFind == ap.piece)
            {
                return ap.dir;
            }
        }

        return std::nullopt;
    }

    sf::Keyboard::Key Surroundings::dirOfPiece(const Piece pieceToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (pieceToFind == ap.piece)
            {
                return ap.dir;
            }
        }

        return keys::not_a_key;
    }

    BoardPosOpt_t Surroundings::posOfPieceOpt(const Piece pieceToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (pieceToFind == ap.piece)
            {
                return ap.pos;
            }
        }

        return std::nullopt;
    }

    BoardPos_t Surroundings::posOfPiece(const Piece pieceToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (pieceToFind == ap.piece)
            {
                return ap.pos;
            }
        }

        return BoardPosInvalid;
    }

    PieceEnumOpt_t Surroundings::pieceAtPosOpt(const BoardPos_t & posToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (posToFind == ap.pos)
            {
                return ap.piece;
            }
        }

        return std::nullopt;
    }

    DirKeyOpt_t Surroundings::dirOfPosOpt(const BoardPos_t & posToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (posToFind == ap.pos)
            {
                return ap.dir;
            }
        }

        return std::nullopt;
    }

    sf::Keyboard::Key Surroundings::dirOfPos(const BoardPos_t & posToFind) const
    {
        for (const AdjacentInfo & ap : adjacents)
        {
            if (posToFind == ap.pos)
            {
                return ap.dir;
            }
        }

        return keys::not_a_key;
    }

    std::size_t Surroundings::pieceCount(const Piece pieceToFind) const
    {
        std::size_t count{ 0 };
        for (const AdjacentInfo & ap : adjacents)
        {
            if (pieceToFind == ap.piece)
            {
                ++count;
            }
        }

        return count;
    }

    std::size_t Surroundings::piecesCount(const std::initializer_list<Piece> & list) const
    {
        std::size_t count{ 0 };
        for (const Piece piece : list)
        {
            count += pieceCount(piece);
        }

        return count;
    }

} // namespace snake
