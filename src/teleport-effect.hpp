#ifndef SNAKE_TELEPORT_EFFECT_HPP_INCLUDED
#define SNAKE_TELEPORT_EFFECT_HPP_INCLUDED
//
// teleport-effect.hpp
//
#include "check-macros.hpp"
#include "common-types.hpp"
#include "context.hpp"
#include "media.hpp"
#include "random.hpp"
#include "settings.hpp"
#include "util.hpp"

#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{
    struct Context;

    //

    struct Star : public sf::Drawable
    {
        explicit Star(
            const Context & context,
            const sf::Vector2f & position,
            const sf::Color & color,
            const sf::Vector2f & vel = { 0.0f, 0.0f })
        {
            reset(context, position, color, vel);
        }

        void reset(
            const Context & context,
            const sf::Vector2f & position,
            const sf::Color & color,
            const sf::Vector2f & vel = { 0.0f, 0.0f })
        {
            elapsed_sec = 0.0f;
            duration_sec = context.random.fromTo(0.25f, 1.0f);
            velocity = vel;

            dimm_max_size = 4.0f;
            dimm_max_size += (static_cast<float>(context.layout.window_size.x) / 100.0f);

            sprite.setTexture(context.media.starTexture());
            util::setOriginToCenter(sprite);
            sprite.setPosition(position);
            sprite.setScale(0.0f, 0.0f);
            sprite.setColor(color);
        }

        bool isAlive() const { return (ageRatio() < 1.0f); }
        float ageRatio() const { return std::clamp((elapsed_sec / duration_sec), 0.0f, 1.0f); }
        bool isGrowing() const { return (ageRatio() < 0.5f); }

        void update(Context &, const float elapsedSec)
        {
            if (!isAlive())
            {
                return;
            }

            //
            elapsed_sec += elapsedSec;

            //
            sprite.move(velocity * elapsedSec);

            //
            const float blueColorValueF { static_cast<float>(sprite.getColor().b)
                                          - (ageRatio() * 64.0f) };

            const sf::Uint8 blueColorValue { static_cast<sf::Uint8>(
                std::clamp(blueColorValueF, 0.0f, 255.0f)) };

            sf::Color slowlyMoreYellowColor { sprite.getColor() };
            slowlyMoreYellowColor.b = blueColorValue;

            sprite.setColor(slowlyMoreYellowColor);

            //
            if (isGrowing())
            {
                const float sizeRatio { 2.0f * ageRatio() };

                const sf::Vector2f currentSize { sizeRatio
                                                 * sf::Vector2f(dimm_max_size, dimm_max_size) };

                util::fit(sprite, currentSize);
            }
            else
            {
                const float sizeRatio { 1.0f - ageRatio() };

                const sf::Vector2f currentSize { sizeRatio
                                                 * sf::Vector2f(dimm_max_size, dimm_max_size) };

                util::fit(sprite, currentSize);
            }

            if (!isAlive())
            {
                sprite.setScale(0.0f, 0.0f);
            }
        }

        void draw(sf::RenderTarget & target, sf::RenderStates) const override
        {
            if (!isAlive())
            {
                return;
            }

            target.draw(sprite);
            target.draw(sprite, sf::BlendAdd);
        }

        float elapsed_sec;
        float duration_sec;
        float dimm_max_size;
        sf::Vector2f velocity;
        sf::Sprite sprite;
    };

    //

    struct SparkleArea
    {
        SparkleArea(
            const BoardPos_t & pos,
            const sf::FloatRect & bou,
            const sf::Color & col,
            const sf::Vector2f & vel = { 0.0f, 0.0f })
            : position(pos)
            , bounds(bou)
            , color(col)
            , velocity(vel)
        { }

        BoardPos_t position;
        sf::FloatRect bounds;
        sf::Color color;
        sf::Vector2f velocity;
    };

    //

    class TeleportEffect : public sf::Drawable
    {
    public:
        TeleportEffect() = default;

        TeleportEffect(const Context & context) { reset(context); }

        void reset(const Context &)
        {
            m_stars.clear();
            m_stars.reserve(1000);

            m_areas.clear();
            m_areas.reserve(100);

            sec_until_spawn = 0.0f;
            elapsed_sec = 0.0f;
        }

        void update(Context & context, const float elpasedTimeSec)
        {
            elapsed_sec += elpasedTimeSec;
            if (elapsed_sec > sec_until_spawn)
            {
                elapsed_sec = 0.0f;
                sec_until_spawn = context.random.fromTo(0.1f, 0.5f);

                for (const SparkleArea & area : m_areas)
                {
                    const float spawnPosLeft { area.bounds.left
                                               + context.random.zeroTo(area.bounds.width) };

                    const float spawnPosTop { area.bounds.top
                                              + context.random.zeroTo(area.bounds.height) };

                    const sf::Vector2f spawnPos { spawnPosLeft, spawnPosTop };

                    bool alreadyCreated { false };
                    for (Star & star : m_stars)
                    {
                        if (!star.isAlive())
                        {
                            star.reset(context, spawnPos, area.color, area.velocity);
                            alreadyCreated = true;
                            break;
                        }
                    }

                    if (!alreadyCreated)
                    {
                        m_stars.emplace_back(context, spawnPos, area.color, area.velocity);
                    }
                }
            }

            for (Star & star : m_stars)
            {
                star.update(context, elpasedTimeSec);
            }
        }

        void draw(sf::RenderTarget & target, sf::RenderStates states) const override //-V813
        {
            for (const Star & star : m_stars)
            {
                target.draw(star, states);
            }
        }

        void
            add(const Context & context,
                const BoardPos_t & boardPos,
                const sf::Color & color = sf::Color::White,
                const sf::Vector2f & velocity = { 0.0f, 0.0f })
        {
            // make the bounds bigger so that some of the sparkles are just past the edges
            const sf::FloatRect bounds { util::scaleRectInPlaceCopy(
                context.layout.cellBounds(boardPos), 2.0f) };

            add(bounds, color, velocity);
        }

        void
            add(const sf::FloatRect & bounds,
                const sf::Color & color = sf::Color::White,
                const sf::Vector2f & velocity = { 0.0f, 0.0f })
        {
            m_areas.emplace_back(BoardPosInvalid, bounds, color, velocity);
        }

        void remove(const BoardPos_t & boardPosToRemove)
        {
            m_areas.erase(
                std::remove_if(
                    std::begin(m_areas),
                    std::end(m_areas),
                    [&](const SparkleArea & area) { return (area.position == boardPosToRemove); }),
                std::end(m_areas));
        }

    private:
        std::vector<SparkleArea> m_areas;
        std::vector<Star> m_stars;
        float sec_until_spawn { 0.0f };
        float elapsed_sec { 0.0f };
    };
} // namespace snake

#endif // #define SNAKE_TELEPORT_EFFECT_HPP_INCLUDED
