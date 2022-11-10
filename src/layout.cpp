// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// layout.cpp
//
#include "layout.hpp"

#include "check-macros.hpp"
#include "context.hpp"
#include "random.hpp"
#include "settings.hpp"
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

} // namespace snake
