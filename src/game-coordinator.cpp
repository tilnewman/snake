// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// game-coordinator.cpp
//
#include "game-coordinator.hpp"

#include "util.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>

#include <SFML/System/Clock.hpp>

namespace snake
{
    GameCoordinator::GameCoordinator(const GameConfig & configOrig)
        : m_configOriginalCopy(configOrig)
        , m_config()
        , m_layout()
        , m_media()
        , m_game()
        , m_window()
        , m_bloomWindow()
        , m_board()
        , m_random()
        , m_soundPlayer(m_random)
        , m_animationPlayer(m_random)
        , m_cellAnims()
        , m_statusRegion()
        , m_teleportEffect()
        , m_stateMachine()
        , m_context(
              m_config,
              m_layout,
              m_game,
              m_media,
              m_board,
              m_random,
              m_soundPlayer,
              m_animationPlayer,
              m_cellAnims,
              m_stateMachine,
              m_statusRegion,
              m_teleportEffect)
        , m_runClock()
    {}

    void GameCoordinator::setup(const GameConfig & configParam)
    {
        // don't call m_config::reset() because that would erase all customizations in configParam
        m_config = configParam;

        openWindow();

        if (m_config.resolution != m_window.getSize())
        {
            std::cout << "The window was not the size expected, so a bunch of calculations have to "
                         "be redone.  ("
                      << m_config.resolution << " to " << m_window.getSize() << ")" << std::endl;

            m_config.resolution = m_window.getSize();
        }

        m_config.verifyAllMembers();

        m_layout.reset(m_config);
        m_media.reset(m_config.media_path);
        m_game.reset(m_config, m_layout);
        m_board.reset();
        m_cellAnims.reset();
        m_animationPlayer.reset((m_config.media_path / "animation").string());
        m_soundPlayer.reset((m_config.media_path / "sfx").string());

        m_soundPlayer.load({
            "rpg-game-over.ogg",     // death
            "explode-puff.ogg",      // text message appear
            "tap-1-a.ogg",           // keystroke/turn
            "miss.ogg",              // miss food by one space
            "shine.ogg",             // eat food
            "step-smash-yuck.ogg",   // eat self
            "mario-pause.ogg",       // pause
            "mario-break-block.ogg", // wall hit
            "level-intro.ogg"        // start level
        });

        m_soundPlayer.volume(m_config.initial_volume);

        if (m_config.isTest())
        {
            // when vol is zero the SoundPlayer class early exits all functions,
            // so this means it won't take up any time and distort profiling results.
            m_soundPlayer.volume(0.0f);
        }

        m_stateMachine.reset();
        m_statusRegion.reset(m_context);
        m_teleportEffect.reset(m_context);

        if (m_config.isTest())
        {
            m_stateMachine.setChangePending(State::Test);
        }
        else
        {
            m_stateMachine.setChangePending(State::Option);
        }
    }

    void GameCoordinator::openWindow()
    {
        const sf::VideoMode videoMode(
            m_config.resolution.x,
            m_config.resolution.y,
            sf::VideoMode::getDesktopMode().bitsPerPixel);

        m_window.create(videoMode, "Snake", m_config.sf_window_style);

        M_CHECK_SS(m_window.isOpen(), "Failed to open a window with these settings: " << videoMode);

        // verify the window size is what was specified/expected,
        // otherwise all the size/positions calculations will be wrong
        const sf::Vector2u windowExpectedSize{ m_config.resolution };
        const sf::Vector2u windowActualSize{ m_window.getSize() };

        if (windowActualSize != windowExpectedSize)
        {
            std::cout << "The window opened okay but not at the size specified: " << videoMode << ""
                      << windowActualSize << ".  So...meh.  Let's just run with it." << std::endl;
        }

        if (m_config.sf_window_style != sf::Style::None)
        {
            const auto desktopVideoMode{ sf::VideoMode::getDesktopMode() };
            const sf::Vector2u desktopScreenSize(desktopVideoMode.width, desktopVideoMode.height);
            if (windowActualSize != desktopScreenSize)
            {
                std::cout << "The window opened okay at the resolution intended ("
                          << windowActualSize << ") but somehow the desktop resolution ("
                          << desktopVideoMode
                          << ") is different...?  So... meh.Let's just run with it." << std::endl;
            }
        }

        m_window.setFramerateLimit(m_config.frame_rate_limit);

        if (!m_bloomWindow)
        {
            m_bloomWindow = std::make_unique<util::BloomEffectHelper>(m_window);
        }

        if (!m_config.isTest())
        {
            m_bloomWindow->isEnabled(true);
            m_bloomWindow->blurMultipassCount(5); // 5 looks like the max before fading out
        }
    }

    void GameCoordinator::play(const GameConfig & config)
    {
        if (config.is_all_video_mode_test)
        {
            repeatPlayForAllVideoModes(config);
        }
        else
        {
            setup(config);
            frameLoop();
        }
    }

    void GameCoordinator::repeatPlayForAllVideoModes(const GameConfig & configOrig)
    {
        const unsigned int desktopBitsPerPixel{ sf::VideoMode::getDesktopMode().bitsPerPixel };

        std::vector<sf::VideoMode> videoModes{ sf::VideoMode::getFullscreenModes() };
        std::reverse(std::begin(videoModes), std::end(videoModes));

        for (const sf::VideoMode & vm : videoModes)
        {
            std::cout << "Testing at video mode: " << vm;

            if (vm.bitsPerPixel != desktopBitsPerPixel)
            {
                std::cout << " -skipped because the bits_per_pixel is different.\n";
                continue;
            }

            std::cout << '\n';

            GameConfig configForThisTest{ configOrig };
            configForThisTest.resolution.x = vm.width;
            configForThisTest.resolution.y = vm.height;

            setup(configForThisTest);
            frameLoop();

            // remove old events
            sf::Event event;
            while (m_window.pollEvent(event))
            {
            }
        }
    }

    void GameCoordinator::frameLoop()
    {
        sf::Clock frameClock;
        sf::Clock periodClock;
        std::size_t frameCounter{ 0 };

        m_runClock.restart();

        while (willContinue())
        {
            handlePeriodicTasks(periodClock, frameCounter);
            handleEvents();
            update(frameClock.restart().asSeconds());
            draw();
            m_stateMachine.changeIfPending(m_context);
        }

        if (m_config.isTest())
        {
            printDebugStatus();
        }
    }

    void GameCoordinator::handlePeriodicTasks(sf::Clock & periodClock, std::size_t & frameCounter)
    {
        ++frameCounter;

        const float elapsedSec{ periodClock.getElapsedTime().asSeconds() };
        if (elapsedSec < 1.0f)
        {
            return;
        }

        const float fps{ static_cast<float>(frameCounter) / elapsedSec };

        frameCounter = 0;
        periodClock.restart();

        m_context.fps = static_cast<std::size_t>(std::roundf(fps));
        m_statusRegion.updateText(m_context);

        m_cellAnims.cleanup();

        // There are just too many ways for food to either be destroyed or unreachable.
        // So this check prevents players from being trapped in a level requiring them to eat food
        // that cannot be reached.
        if ((m_stateMachine.stateEnum() == State::Play) && !m_game.isGameOver() &&
            !m_game.isLevelComplete())
        {
            if ((m_game.level().remainingToEat() > 0) && (m_board.countPieces(Piece::Food) == 0))
            {
                m_board.addNewPieceAtRandomFreePos(m_context, Piece::Food);
            }
        }
    }

    void GameCoordinator::handleEvents()
    {
        sf::Event event;

        while (willContinue() && m_window.pollEvent(event))
        {
            m_stateMachine.state().handleEvent(m_context, event);
        }
    }

    void GameCoordinator::update(const float elapsedSec)
    {
        m_stateMachine.state().update(m_context, elapsedSec);
    }

    void GameCoordinator::draw()
    {
        m_bloomWindow->clear(m_config.window_background_color);
        m_stateMachine.state().draw(m_context, m_bloomWindow->renderTarget(), sf::RenderStates());
        m_bloomWindow->display();
    }

    void GameCoordinator::printDebugStatus()
    {
        const float runTimeSec{ std::round(m_runClock.getElapsedTime().asSeconds() * 100.0f) /
                                100.0f };

        std::cout << std::endl;

        std::cout << m_config.toString() << "\n\n";
        std::cout << m_layout.toString() << "\n\n";
        std::cout << m_board.toString(m_context) << "\n\n";
        std::cout << m_game.toString() << "\n\n";

        std::cout << "Play Time: " << runTimeSec << "sec" << std::endl;
    }
} // namespace snake
