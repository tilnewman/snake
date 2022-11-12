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
//  lives counter
//  earn more lives with score
//  food should always eventually die and be respawned
//  food increasingly away from middle toward walls/self/ect
//  bonus levels with lots of food but your tail never stops growing
//  bonuses for turns with limited options or that avoided head-on collision
//  bonus for long straight shots
//  spinning food that turns you when you eatit in the dir it was spinning
//  hidden food that gives big score bonus when eaten

//
// Bugs
//
//  MacOS resolutions are screwed up

int main(const int argc, const char * const argv[])
{
    using namespace snake;

    GameConfig config;
    config.media_path = ((argc > 1) ? argv[1] : "no_media_folder");
    std::cout << "media_path=" << config.media_path << std::endl;

    config.frame_rate_limit = 0;
    config.is_god_mode = false;

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
