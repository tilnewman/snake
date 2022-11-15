#ifndef SNAKE_CONTEXT_HPP_INCLUDED
#define SNAKE_CONTEXT_HPP_INCLUDED
//
// context.hpp
//

namespace util
{
    class Random;
    class SoundPlayer;
    class AnimationPlayer;
} // namespace util

namespace snake
{
    class Board;
    class Media;
    class GameConfig;
    class GameInPlay;
    class Animations;
    struct IStatesPending;
    class Layout;
    struct IRegion;
    class ScoreFile;

    //

    struct Context
    {
        Context(
            const GameConfig & con,
            const Layout & lay,
            GameInPlay & gam,
            const Media & med,
            Board & bor,
            util::Random & ran,
            util::SoundPlayer & aud,
            util::AnimationPlayer & ani,
            Animations & cellAnims,
            IStatesPending & sta,
            IRegion & stat,
            ScoreFile & scoreFile)
            : config(con)
            , layout(lay)
            , game(gam)
            , media(med)
            , board(bor)
            , random(ran)
            , audio(aud)
            , anim(ani)
            , cell_anims(cellAnims)
            , state(sta)
            , status(stat)
            , score_file(scoreFile)
        {}

        Context(const Context &) = delete;
        Context(Context &&) = delete;

        Context & operator=(const Context &) = delete;
        Context & operator=(Context &&) = delete;

        const GameConfig & config;
        const Layout & layout;
        GameInPlay & game;
        const Media & media;
        Board & board;
        const util::Random & random;
        util::SoundPlayer & audio;
        util::AnimationPlayer & anim;
        Animations & cell_anims;
        IStatesPending & state;
        IRegion & status;
        ScoreFile & score_file;

        std::size_t fps{ 0 };
    };
} // namespace snake

#endif // SNAKE_CONTEXT_HPP_INCLUDED
