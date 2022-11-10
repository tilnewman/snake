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
              m_statusRegion)
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
            "level-intro.ogg",       // start level
            "slow.ogg"               // eat slow pill
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

        if (m_config.isTest())
        {
            m_stateMachine.setChangePending(State::Test);
        }
        else
        {
            m_stateMachine.setChangePending(State::Option);
        }
    }

    const sf::VideoMode GameCoordinator::pickResolution() const
    {
        std::vector<sf::VideoMode> videoModes = sf::VideoMode::getFullscreenModes();

        // remove all with different bit depths
        const unsigned int desktopBitsPerPixel{ sf::VideoMode::getDesktopMode().bitsPerPixel };

        videoModes.erase(
            std::remove_if(
                std::begin(videoModes),
                std::end(videoModes),
                [&](const sf::VideoMode & vm) { return (vm.bitsPerPixel != desktopBitsPerPixel); }),
            std::end(videoModes));

        // remove all with resolutions too high
        const unsigned int maxDimmension = 2000;

        videoModes.erase(
            std::remove_if(
                std::begin(videoModes),
                std::end(videoModes),
                [&](const sf::VideoMode & vm) {
                    return ((vm.width > maxDimmension) || (vm.height > maxDimmension));
                }),
            std::end(videoModes));

        // ensure the order is highest resolution first
        std::sort(std::begin(videoModes), std::end(videoModes));
        std::reverse(std::begin(videoModes), std::end(videoModes));

        return *videoModes.begin();
    }

    void GameCoordinator::openWindow()
    {
        const sf::VideoMode videoMode = pickResolution();
        std::cout << "Video Mode Selected: " << videoMode << '\n';

        m_window.create(videoMode, "Snake", m_config.sf_window_style);
        M_CHECK_SS(m_window.isOpen(), "Failed to open a window with these settings: " << videoMode);

        m_config.resolution.x = m_window.getSize().x;
        m_config.resolution.y = m_window.getSize().y;

        m_window.setFramerateLimit(m_config.frame_rate_limit);

        if (!m_bloomWindow)
        {
            m_bloomWindow = std::make_unique<util::BloomEffectHelper>(m_window);
        }

        // don't use bloom effect when testing because it reduces frame rate significantly
        if (!m_config.isTest())
        {
            m_bloomWindow->isEnabled(true);
            m_bloomWindow->blurMultipassCount(5); // 5 looks like the brightest glow
        }
    }

    void GameCoordinator::play(const GameConfig & config)
    {
        setup(config);
        frameLoop();

        if (m_config.isTest())
        {
            printDebugStatus();
        }

        const float runTimeSec{ std::round(m_runClock.getElapsedTime().asSeconds() * 100.0f) /
                                100.0f };

        std::cout << "Play Time: " << runTimeSec << "sec\n";
        std::cout << "Final Score:" << m_game.score() << std::endl;
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

        // Periodically place new food at random place on the map, because there
        // are just too many ways for food to either be destroyed or unreachable.
        if ((m_stateMachine.stateEnum() == State::Play) && !m_game.isGameOver() &&
            !m_game.isLevelComplete())
        {
            if ((m_game.level().remainingToEat() > 0) && (m_board.countPieces(Piece::Food) == 0))
            {
                m_board.addNewPieceAtRandomFreePos(m_context, Piece::Food);

                if (m_game.level().remainingToEat() == 3)
                {
                    m_board.addNewPieceAtRandomFreePos(m_context, Piece::Slow);
                }
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
        std::cout << std::endl;
        std::cout << m_config.toString() << "\n\n";
        std::cout << m_layout.toString() << "\n\n";
        std::cout << m_board.toString(m_context) << "\n\n";
        std::cout << m_game.toString() << std::endl;
    }
} // namespace snake
