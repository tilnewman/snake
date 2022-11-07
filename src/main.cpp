// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// main.cpp
//
#include "game-coordinator.hpp"
#include "settings.hpp"

#include <cstddef>

// *** make sure M_CHECK_ASSERT_SS works in a destructor ***
//
// tails that turn into walls should be gray and not kill but reduce points
//
// food should always eventually die and be respawned
// food increasingly away from middle to intentionally next to wall/self/ect
// poison pills near other food or that appear in your way
// maps have walls
// levels whith lots of food but your tail never shrinks
// bonuses for turns with limited options or that avoided head-on collision
// bonus for long straight shots
// food that moves away from you
// food that bounces
// poison pill that grows like a snake
// all the food for the level is not visible at leel start, but reveals after each is eaten
// spinning food that turns you when you eatit in the dir it was spinning
// pulsing food that fades in-out of transparency

// SHAKE THE SCREEN

// Tiling:
//  * The outermost wall will always be there, so if you want, you'll have to remove it yourself
//     However, if you use borders > 0 then you may find it already works with an outer wall
//  * Use "irregular" to split into EXACTLY as many total tiles as you specified
//  * Use Uniform/Square to get as close as you can to Uniform/Square:
//      BUT they are rarely exactly Uniform/Square because they are working with ints and not floats
//      BUT often return more tiles than speciified to gret as close to Uniform/Square as possible
//  * Don't set the tiles to be filled in unless you also set a border>0 or forcedSquare as well
//
//  Single/Line/Row/Column
//

//
// Bugs
//

// remove shapes_rot
// remove stats.list_str?

//

int main(const int argc, const char * const argv[])
{
    using namespace snake;

    GameConfig config;
    config.media_path = ((argc > 1) ? argv[1] : "no_media_folder");
    std::cout << "media_path=\"" << config.media_path << '\"' << std::endl;
    config.frame_rate_limit = 0;
    // config.resolution = { 1600, 1200 };
    config.cell_size_window_ratio = 0.01f;
    config.sf_window_style = sf::Style::Fullscreen;
    config.is_level_test = false;
    config.is_level_test_manual = false;
    config.is_god_mode = false;
    // config.is_all_video_mode_test = false;

    GameCoordinator game(config);

    try
    {
        game.play();
    }
    catch (const std::exception & ex)
    {
        std::cout << "EXCEPTION ERROR: DURING GAMEPLAY:  \"" << ex.what() << "\"" << std::endl;
    }
    catch (...)
    {
        std::cout << "EXCEPTION ERROR: DURING GAMEPLAY: \"UNKOWNN\"" << std::endl;
    }

    return EXIT_SUCCESS;
}
