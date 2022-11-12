#ifndef SNAKE_STATES_HPP_INCLUDED
#define SNAKE_STATES_HPP_INCLUDED
//
// states.hpp
//
#include "check-macros.hpp"
#include "context.hpp"
#include "keys.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

namespace sf
{
    class Text;
    class Event;
    class RenderTarget;
    class RenderStates;
} // namespace sf

namespace snake
{
    struct Context;
    class GameConfig;
    class Media;

    //

    enum class State
    {
        Start = 0, // an empty "do-nothing" placeholder while the app starts up
        Option,    // first thing player sees, allows customizing game before it starts
        Play,
        Pause,
        LevelCompleteMsg, // TimedMessage showing "Level Survived!"
        NextLevelMsg,     // TimedMessage showing what level # is next
        Over,
        Quit // performs all normal shutdown and exits the program
    };

    using StateOpt_t = std::optional<State>;

    //

    namespace state
    {
        inline std::string toString(const State state)
        {
            switch (state)
            {
                case State::Start: return "Start";
                case State::Option: return "Option";
                case State::Play: return "Play";
                case State::Pause: return "Pause";
                case State::LevelCompleteMsg: return "LevelCompleteMsg";
                case State::NextLevelMsg: return "NextLevelMsg";
                case State::Over: return "Over";
                case State::Quit: return "Quit";
                default: return "";
            }
        }
    } // namespace state

    //
    inline std::ostream & operator<<(std::ostream & os, const State state)
    {
        os << state::toString(state);
        return os;
    }

    //
    struct IState
    {
        virtual ~IState() = default;

        virtual State state() const = 0;
        virtual State nextState() const = 0;
        virtual void update(Context &, const float elapsedSec) = 0;
        virtual bool handleEvent(Context & context, const sf::Event & event) = 0;
        virtual void draw(const Context &, sf::RenderTarget &, const sf::RenderStates &) const = 0;
        virtual void onEnter(Context &) = 0;
        virtual void onExit(Context &) = 0;

      protected:
        virtual bool changeToNextState(const Context &) = 0;
        virtual bool willIgnoreEvent(const Context &, const sf::Event & event) const = 0;

        // returns true if the event was a 'quit' event and a state changed is pneding
        virtual bool handleQuitEvents(Context & context, const sf::Event & event) = 0;
    };

    using IStateUPtr_t = std::unique_ptr<IState>;

    //
    class StateBase : public IState
    {
      protected:
        StateBase(const State state, const State nextState, const float minDurationSec = -1.0f);

        StateBase(
            const Context & context,
            const State state,
            const State nextState,
            const std::string & message = {},
            const float minDurationSec = -1.0f);

      public:
        virtual ~StateBase() override = default;

        // prevent all copy and assignment
        StateBase(const StateBase &) = delete;
        StateBase(StateBase &&) = delete;
        //
        StateBase & operator=(const StateBase &) = delete;
        StateBase & operator=(StateBase &&) = delete;

        State state() const final { return m_state; }
        State nextState() const final { return m_nextState; }
        void update(Context &, const float elapsedSec) override;
        bool handleEvent(Context & context, const sf::Event & event) override;
        void draw(const Context &, sf::RenderTarget &, const sf::RenderStates &) const override;
        void onEnter(Context &) override {}
        void onExit(Context &) override {}

      protected:
        bool hasMinTimeElapsed() const
        {
            return (!(m_minDurationSec > 0.0f) || (m_elapsedTimeSec > m_minDurationSec));
        }

        bool changeToNextState(const Context &) override;
        bool willIgnoreEvent(const Context &, const sf::Event & event) const override;
        bool handleQuitEvents(Context &, const sf::Event &) override;
        void setupText(const Context & context, const std::string & message);
        // void updateBgFade(const float elapsedSec);

      protected:
        State m_state;
        State m_nextState;
        float m_elapsedTimeSec;
        float m_minDurationSec; // any negative means this value is ignored
        sf::Text m_text;

        static inline const sf::Color m_textColorDefault{ sf::Color(200, 200, 200) };

        static inline const float m_defaultMinDurationSec{ 1.5f };
    };

    // the initial state that only  to State::Options
    struct StartState : public StateBase
    {
        StartState()
            : StateBase(State::Start, State::Option)
        {}

        virtual ~StartState() override = default;

        void update(Context &, const float) final {}
        bool handleEvent(Context &, const sf::Event &) final { return false; }
        void draw(const Context &, sf::RenderTarget &, const sf::RenderStates &) const final {}
    };

    // the state that simply exits the application
    struct QuitState : public StateBase
    {
        QuitState()
            : StateBase(State::Quit, State::Quit)
        {}

        virtual ~QuitState() override = default;

        void update(Context &, const float) final {}
        bool handleEvent(Context &, const sf::Event &) final { return false; }
        void draw(const Context &, sf::RenderTarget &, const sf::RenderStates &) const final {}
    };

    //
    struct OptionsState : public StateBase
    {
        explicit OptionsState(Context & context);
        virtual ~OptionsState() override = default;

        void update(Context &, const float elapsedSec) override;
        bool handleEvent(Context & context, const sf::Event & event) override;
        void onEnter(Context &) override;
    };

    //
    struct TimedMessageState : public StateBase
    {
        explicit TimedMessageState(
            const Context & context,
            const State state,
            const State nextState,
            const std::string & message,
            const float minDurationSec = StateBase::m_defaultMinDurationSec);

        virtual ~TimedMessageState() override = default;

        void update(Context & context, const float elapsedSec) override;
        bool handleEvent(Context & context, const sf::Event & event) override;

      protected:
        bool m_hasMouseClickedOrKeyPressed{ false };
    };

    //
    struct LevelCompleteMessageState : public TimedMessageState
    {
        explicit LevelCompleteMessageState(const Context & context);
        virtual ~LevelCompleteMessageState() override = default;

        void onEnter(Context &) override;
        void onExit(Context &) override;
    };

    //
    struct NextLevelMessageState : public TimedMessageState
    {
        explicit NextLevelMessageState(const Context & context);
        virtual ~NextLevelMessageState() override = default;

        void onEnter(Context &) override;

        static std::string makeMessage(const Context & context);
    };

    //
    struct GameOverState : public TimedMessageState
    {
        explicit GameOverState(const Context & context);
        virtual ~GameOverState() override = default;

        void onEnter(Context &) override;
        void onExit(Context &) override;
    };

    //
    struct PauseState : public TimedMessageState
    {
        explicit PauseState(const Context & context);
        virtual ~PauseState() override = default;

        void onEnter(Context &) override;
        void update(Context & context, const float elapsedSec) override;
    };

    //
    class PlayState : public StateBase
    {
      public:
        explicit PlayState(const Context & context);

        virtual ~PlayState() override = default;

        void onEnter(Context &) override;
        bool handleEvent(Context &, const sf::Event &) override;
        void update(Context & context, const float elapsedSec) override;
    };

    //
    struct IStatesPending
    {
        virtual ~IStatesPending() = default;

        virtual bool isChangePending() const = 0;
        virtual StateOpt_t getChangePending() const = 0;
        virtual void setChangePending(const State state) = 0;
    };

    //
    class StateMachine : public IStatesPending
    {
      public:
        StateMachine();
        virtual ~StateMachine() override = default;

        // prevent all copy and assignment
        StateMachine(const StateMachine &) = delete;
        StateMachine(StateMachine &&) = delete;
        //
        StateMachine & operator=(const StateMachine &) = delete;
        StateMachine & operator=(StateMachine &&) = delete;

        void reset();

        State stateEnum() const { return m_stateUPtr->state(); }

        IState & state() { return *m_stateUPtr; }
        const IState & state() const { return *m_stateUPtr; }

        bool isChangePending() const override { return m_changePendingOpt.has_value(); }
        StateOpt_t getChangePending() const override { return m_changePendingOpt; }
        void setChangePending(const State state) override;
        void changeIfPending(Context & context);

      private:
        IStateUPtr_t makeState(Context & context, const State state);

      private:
        IStateUPtr_t m_stateUPtr;
        StateOpt_t m_changePendingOpt;
    };
} // namespace snake

#endif // SNAKE_STATES_HPP_INCLUDED
