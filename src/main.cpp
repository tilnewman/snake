// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// main.cpp
//
#include "game-coordinator.hpp"
#include "settings.hpp"

#include <cstddef>

//
// TODO
//
//  temp cheater key to skip levels to make level changes easy to see/test
//  level details depend on time spent playing so more time == harder levels
//  food increasingly away from middle toward walls/self/ect
//  bonus levels with lots of food but your tail never stops growing
//  bonuses for turns with limited options or that avoided head-on collision
//  bonus for long straight shots
//  hidden food that gives big score bonus when eaten
//  save max play time and max score in a file and display during option state

//
// Bugs
//

int main(const int argc, const char * const argv[])
{
    using namespace snake;

    GameConfig config;
    config.media_path = ((argc > 1) ? argv[1] : "no_media_folder");

    if (argc > 2)
    {
        config.will_limit_resolution = ("limit-resolution" == std::string{ argv[2] });
    }

    config.frame_rate_limit = 0;
    config.is_god_mode = false;
    config.will_show_fps = true;

    try
    {
        GameCoordinator game(config);
        game.play();
    }
    catch (const std::exception & ex)
    {
        std::cout << "EXCEPTION ERROR:  \"" << ex.what() << "\"" << std::endl;
    }
    catch (...)
    {
        std::cout << "EXCEPTION ERROR: \"UNKOWNN\"" << std::endl;
    }

    return EXIT_SUCCESS;
}
