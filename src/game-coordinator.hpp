#ifndef SNAKE_GAMECOORDINATOR_HPP_INCLUDED
#define SNAKE_GAMECOORDINATOR_HPP_INCLUDED

#include "animation-player.hpp"
#include "bloom-shader.hpp"
#include "board.hpp"
#include "cell-animations.hpp"
#include "context.hpp"
#include "media.hpp"
#include "pieces.hpp"
#include "random.hpp"
#include "settings.hpp"
#include "sound-player.hpp"
#include "states.hpp"
#include "status-region.hpp"

#include <memory>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>

namespace snake
{
    class GameCoordinator
    {
      public:
        GameCoordinator(const GameConfig & configOrig);

        // prevent all copy and assignment
        GameCoordinator(const GameCoordinator &) = delete;
        GameCoordinator(GameCoordinator &&) = delete;
        //
        GameCoordinator & operator=(const GameCoordinator &) = delete;
        GameCoordinator & operator=(GameCoordinator &&) = delete;

        void play() { play(m_configOriginalCopy); }
        void play(const GameConfig & config);
        void printDebugStatus();

      private:
        bool willContinue() const
        {
            return (m_window.isOpen() && (m_stateMachine.stateEnum() != State::Quit));
        }

        void repeatPlayForAllVideoModes(const GameConfig & config);
        void frameLoop();
        void setup(const GameConfig & config);
        const sf::VideoMode pickResolution() const;
        void openWindow();
        void handlePeriodicTasks(sf::Clock & periodClock, std::size_t & frameCounter);
        void handleEvents();
        void update(const float elapsedSec);
        void draw();

      private:
        GameConfig m_configOriginalCopy;
        GameConfig m_config;
        Layout m_layout;
        Media m_media;
        GameInPlay m_game;
        sf::RenderWindow m_window;
        std::unique_ptr<util::BloomEffectHelper> m_bloomWindow;
        Board m_board;
        util::Random m_random;
        util::SoundPlayer m_soundPlayer;
        util::AnimationPlayer m_animationPlayer;
        Animations m_cellAnims;
        StatusRegion m_statusRegion;
        StateMachine m_stateMachine;
        Context m_context;

        sf::Clock m_runClock;
    };
} // namespace snake

#endif // SNAKE_GAMECOORDINATOR_HPP_INCLUDED
