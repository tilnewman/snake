// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// states.cpp
//
#include "states.hpp"

#include "animation-player.hpp"
#include "board.hpp"
#include "cell-animations.hpp"
#include "layout.hpp"
#include "media.hpp"
#include "pieces.hpp"
#include "random.hpp"
#include "score-file.hpp"
#include "settings.hpp"
#include "sound-player.hpp"
#include "status-region.hpp"
#include "util.hpp"

#include <sstream>

namespace snake
{
    StateBase::StateBase(const State state, const State nextState, const float minDurationSec)
        : m_state(state)
        , m_nextState(nextState)
        , m_elapsedTimeSec(0.0f)
        , m_minDurationSec(minDurationSec) // any negative means this value is ignored
    {}

    StateBase::StateBase(
        const Context & context,
        const State state,
        const State nextState,
        const std::string & message,
        const float minDurationSec)
        : StateBase(state, nextState, minDurationSec)
    {
        setupText(context, message);
    }

    void StateBase::setupText(const Context & context, const std::string & message)
    {
        m_text.setString(message);
        m_text.setCharacterSize(99);
        m_text.setFont(context.media.font());
        m_text.setFillColor(m_textColorDefault);

        util::fitAndCenterInside(
            m_text, util::scaleRectInPlaceCopy(context.layout.window_bounds_f, 0.25f));
    }

    void StateBase::update(Context & context, const float elapsedSec)
    {
        m_elapsedTimeSec += elapsedSec;
        context.cell_anims.update(context, elapsedSec);
    }

    bool StateBase::changeToNextState(const Context & context)
    {
        if (state() == m_nextState)
        {
            return false;
        }

        context.state.setChangePending(nextState());
        return true;
    }

    bool StateBase::willIgnoreEvent(const Context & context, const sf::Event & event) const
    {
        // all events should be ignored after a state change is scheduled
        if (context.state.isChangePending())
        {
            return true;
        }

        // clang-format off
        return ((sf::Event::KeyReleased == event.type) ||
                (sf::Event::MouseMoved == event.type) ||
                (sf::Event::MouseEntered == event.type) ||
                (sf::Event::MouseLeft == event.type) ||
                (sf::Event::MouseButtonReleased == event.type) ||
                (sf::Event::GainedFocus == event.type) ||
                (sf::Event::LostFocus == event.type) ||
                (sf::Event::TouchBegan == event.type) ||
                (sf::Event::TouchEnded == event.type) ||
                (sf::Event::TouchMoved == event.type));
        // clang-format on
    }

    bool StateBase::handleQuitEvents(Context & context, const sf::Event & event)
    {
        if (sf::Event::Closed == event.type)
        {
            std::cout << "Player closed the window." << std::endl;
            context.state.setChangePending(State::Quit);
            return true;
        }

        if (state() == State::Pause)
        {
            context.state.setChangePending(State::Play);
            return true;
        }

        // all that remain are keystrokes
        if (sf::Event::KeyPressed != event.type)
        {
            return false;
        }

        if (sf::Keyboard::Q == event.key.code)
        {
            if (state() == State::Play)
            {
                std::cout << "Player pressed 'Q'.  Quitting the current game in play." << std::endl;
                context.state.setChangePending(State::Over);
            }
            else
            {
                std::cout << "Player pressed 'Q', but was not playing a game, so just shutdown."
                          << std::endl;

                context.state.setChangePending(State::Quit);
            }

            return true;
        }

        if (sf::Keyboard::Escape == event.key.code)
        {
            std::cout << "Player pressed 'Escape'.  Shutting down the game." << std::endl;
            context.state.setChangePending(State::Quit);
            return true;
        }

        return false;
    }

    bool StateBase::handleEvent(Context & context, const sf::Event & event)
    {
        if (willIgnoreEvent(context, event))
        {
            return true;
        }

        if (handleQuitEvents(context, event))
        {
            return true;
        }

        return context.state.isChangePending();
    }

    void StateBase::draw(
        const Context & context, sf::RenderTarget & target, const sf::RenderStates & states) const
    {
        context.board.draw(context, target, states);

        target.draw(context.anim, states);
        target.draw(context.cell_anims, states);
        context.status.draw(context, target, states);
        target.draw(m_text, states);
    }

    //

    OptionsState::OptionsState(Context & context)
        : StateBase(
              context,
              State::Option,
              State::NextLevelMsg,
              "Ready?\nHit a key to start!\n\n\n\n",
              m_defaultMinDurationSec)
    {}

    void OptionsState::onEnter(Context &) {}

    void OptionsState::update(Context & context, const float elapsedSec)
    {
        StateBase::update(context, elapsedSec);
    }

    bool OptionsState::handleEvent(Context & context, const sf::Event & event)
    {
        if (StateBase::handleEvent(context, event))
        {
            return true;
        }

        if ((sf::Event::KeyPressed != event.type) && (sf::Event::MouseButtonPressed != event.type))
        {
            return false;
        }

        std::cout << "Player either key-pressed key or mouse-clicked to leave the Option state "
                     "and start playing."
                  << std::endl;

        context.game.start(context);
        changeToNextState(context);

        return true;
    }

    //

    TimedMessageState::TimedMessageState(
        const Context & context,
        const State state,
        const State nextState,
        const std::string & message,
        const float minDurationSec)
        : StateBase(context, state, nextState, message, minDurationSec)
    {
        const sf::FloatRect textBounds{ util::scaleRectInPlaceCopy(
            context.layout.board_bounds_f, 0.9f) };

        util::centerInside(m_text, textBounds);
    }

    bool TimedMessageState::handleEvent(Context & context, const sf::Event & event)
    {
        if (StateBase::handleEvent(context, event))
        {
            return true;
        }

        if ((event.type == sf::Event::KeyPressed) || (event.type == sf::Event::MouseButtonPressed))
        {
            m_hasMouseClickedOrKeyPressed = true;
        }

        if (!hasMinTimeElapsed())
        {
            return false;
        }

        if (m_hasMouseClickedOrKeyPressed)
        {
            changeToNextState(context);
            return true;
        }

        return false;
    }

    void TimedMessageState::update(Context & context, const float elapsedSec)
    {
        StateBase::update(context, elapsedSec);

        if (hasMinTimeElapsed() && m_hasMouseClickedOrKeyPressed)
        {
            changeToNextState(context);
        }
    }

    //

    LevelCompleteMessageState::LevelCompleteMessageState(const Context & context)
        : TimedMessageState(
              context,
              State::LevelCompleteMsg,
              State::NextLevelMsg,
              "Level Survived!",
              (m_defaultMinDurationSec * 2.0f))
    {}

    void LevelCompleteMessageState::onEnter(Context &)
    {
        // std::cout << context.game.statusString("Level Complete") << std::endl;
    }

    void LevelCompleteMessageState::onExit(Context & context)
    {
        context.cell_anims.reset();
        context.game.setupNextLevel(context, true);
    }

    //

    NextLevelMessageState::NextLevelMessageState(const Context & context)
        : TimedMessageState(
              context,
              State::NextLevelMsg,
              State::Play,
              makeMessage(context),
              m_defaultMinDurationSec)
    {}

    void NextLevelMessageState::onEnter(Context & context) { context.audio.play("level-intro"); }

    std::string NextLevelMessageState::makeMessage(const Context & context)
    {
        return ("Level #" + std::to_string(context.game.level().number));
    }

    //

    GameOverState::GameOverState(const Context & context)
        : TimedMessageState(
              context, State::Over, State::NextLevelMsg, "You Died\nTry Again!\n\n", 4.5f)
    {}

    void GameOverState::onEnter(Context & context) { context.audio.play("rpg-game-over"); }

    void GameOverState::onExit(Context & context)
    {
        if (context.game.lives() == 0)
        {
            // save high score if needed
            const int currentHighScore = context.score_file.readHighScore();
            if (context.game.score() > currentHighScore)
            {
                std::cout << "You beat the high score of " << currentHighScore << " by "
                          << (context.game.score() - currentHighScore) << "!\n";

                context.score_file.writeHighScore(context.game.score());
            }

            context.state.setChangePending(State::Quit);
        }
        else
        {
            context.game.setupNextLevel(context, false);
        }
    }

    PauseState::PauseState(const Context & context)
        : TimedMessageState(context, State::Pause, State::Play, "PAUSE", -1.0f)
    {}

    void PauseState::onEnter(Context & context) { context.audio.play("mario-pause"); }

    void PauseState::update(Context & context, const float elapsedSec)
    {
        // don't call StateBase's or TimedMessageState's update(),
        // because that will keep the animations running
        m_elapsedTimeSec += elapsedSec;

        if (hasMinTimeElapsed() && m_hasMouseClickedOrKeyPressed)
        {
            changeToNextState(context);
        }
    }

    //

    PlayState::PlayState(const Context & context)
        : StateBase(context, State::Play, State::Play)
    {}

    void PlayState::onEnter(Context &)
    {
        // std::cout << context.game.statusString("Play Starting") << std::endl;
    }

    void PlayState::update(Context & context, const float elapsedSec)
    {
        StateBase::update(context, elapsedSec);
        context.board.update(context, elapsedSec);
    }

    bool PlayState::handleEvent(Context & context, const sf::Event & event)
    {
        if (StateBase::handleEvent(context, event))
        {
            return true;
        }

        if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Space))
        {
            context.state.setChangePending(State::Pause);
        }
        else
        {
            context.board.passEventToPieces(context, event);
        }

        return context.state.isChangePending();
    }

    //

    StateMachine::StateMachine()
        : m_stateUPtr()
        , m_changePendingOpt(State::Start)
    {
        reset();
    }

    void StateMachine::reset()
    {
        m_stateUPtr = std::make_unique<StartState>();
        m_changePendingOpt = m_stateUPtr->state();
    }

    void StateMachine::setChangePending(const State state) { m_changePendingOpt = state; }

    void StateMachine::changeIfPending(Context & context)
    {
        if (!m_changePendingOpt)
        {
            return;
        }

        m_stateUPtr->onExit(context);

        m_stateUPtr = makeState(context, m_changePendingOpt.value());
        m_changePendingOpt = std::nullopt;

        m_stateUPtr->onEnter(context);
    }

    IStateUPtr_t StateMachine::makeState(Context & context, const State state)
    {
        // clang-format off
        switch (state)
        {
            case State::Start:            { return std::make_unique<StartState>();                       }
            case State::Option:           { return std::make_unique<OptionsState>(context);              }
            case State::Play:             { return std::make_unique<PlayState>(context);                 }
            case State::Over:             { return std::make_unique<GameOverState>(context);             }
            case State::Pause:            { return std::make_unique<PauseState>(context);                }
            case State::LevelCompleteMsg: { return std::make_unique<LevelCompleteMessageState>(context); }
            case State::NextLevelMsg:     { return std::make_unique<NextLevelMessageState>(context); }
           
            case State::Quit:
            default:                      { return std::make_unique<QuitState>(); }
        };
        // clang-format on
    }
} // namespace snake
