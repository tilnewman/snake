#ifndef SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
#define SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
//
// cell-animations.hpp
//
#include "check-macros.hpp"
#include "settings.hpp"
#include "util.hpp"

#include <vector>

#include <SFML/Graphics.hpp>

//

namespace snake
{
    struct Context;

    //

    class Animations : public sf::Drawable
    {
    public:
        Animations()
        {
            m_growFadeSprites.reserve(100);
            m_growFadeTexts.reserve(100);
            setupCellTexture();
        }

        // prevent all copy and assignment
        Animations(const Animations &) = delete;
        Animations(Animations &&) = delete;
        //
        Animations & operator=(const Animations &) = delete;
        Animations & operator=(Animations &&) = delete;

        void reset() { m_growFadeSprites.clear(); }

        void update(Context &, const float)
        {
            for (sf::Sprite & sprite : m_growFadeSprites)
            {
                const sf::FloatRect newRect { util::scaleRectInPlaceCopy(
                    sprite.getGlobalBounds(), 1.05f) };

                util::fitAndCenterInside(sprite, newRect);

                sprite.setColor(sprite.getColor() - sf::Color(0, 0, 0, 9));
            }

            for (sf::Text & text : m_growFadeTexts)
            {
                text.move(0.0f, -1.0f);
                text.setFillColor(text.getFillColor() - sf::Color(0, 0, 0, 5));
            }
        }

        void draw(sf::RenderTarget & target, sf::RenderStates states) const override //-V813
        {
            for (const sf::Sprite & sprite : m_growFadeSprites)
            {
                target.draw(sprite, states);
            }

            for (const sf::Text & text : m_growFadeTexts)
            {
                target.draw(text, states);
            }
        }

        // void handleEvent(Context &, const sf::Event & event);

        void addGrowFadeAnim(const sf::FloatRect & rect, const sf::Color & color)
        {
            sf::Sprite sprite(m_cellSizeTexture);
            sprite.setColor(color);
            util::fitAndCenterInside(sprite, rect);
            m_growFadeSprites.push_back(sprite);
        }

        void addRisingText(
            const Context & context,
            const std::string & message,
            const sf::Color & color,
            const sf::FloatRect & cellBounds);

        void cleanup()
        {
            m_growFadeSprites.erase(
                std::remove_if(
                    std::begin(m_growFadeSprites),
                    std::end(m_growFadeSprites),
                    [&](const sf::Sprite & sprite) { return (sprite.getColor().a == 0); }),
                std::end(m_growFadeSprites));

            m_growFadeTexts.erase(
                std::remove_if(
                    std::begin(m_growFadeTexts),
                    std::end(m_growFadeTexts),
                    [&](const sf::Text & text) { return (text.getFillColor().a == 0); }),
                std::end(m_growFadeTexts));
        }

    private:
        void setupCellTexture()
        {
            sf::Image image;
            image.create(1, 1, sf::Color::White);
            m_cellSizeTexture.loadFromImage(image);
        }

    private:
        sf::Texture m_cellSizeTexture;
        std::vector<sf::Sprite> m_growFadeSprites;
        std::vector<sf::Text> m_growFadeTexts;
    };
} // namespace snake

#endif // #define SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
