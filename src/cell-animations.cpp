// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// cell-animations.cpp
//
#include "cell-animations.hpp"

#include "check-macros.hpp"
#include "context.hpp"
#include "layout.hpp"
#include "media.hpp"
#include "settings.hpp"
#include "util.hpp"

//

namespace snake
{
    Animations::Animations()
        : m_cellSizeTexture()
        , m_growFadeSprites()
        , m_growFadeTexts()
    {
        m_growFadeSprites.reserve(20);
        m_growFadeTexts.reserve(20);
        setupCellTexture();
    }

    void Animations::reset()
    {
        m_growFadeSprites.clear();
        m_growFadeTexts.clear();
    }

    void Animations::update(Context &, const float)
    {
        for (sf::Sprite & sprite : m_growFadeSprites)
        {
            const sf::FloatRect newRect{ util::scaleRectInPlaceCopy(
                sprite.getGlobalBounds(), 1.05f) };

            util::fitAndCenterInside(sprite, newRect);

            sprite.setColor(sprite.getColor() - sf::Color(0, 0, 0, 7));
        }

        for (sf::Text & text : m_growFadeTexts)
        {
            text.move(0.0f, -1.0f);
            text.setFillColor(text.getFillColor() - sf::Color(0, 0, 0, 2));
        }
    }

    void Animations::draw(sf::RenderTarget & target, sf::RenderStates states) const
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

    void Animations::addGrowFadeAnim(const sf::FloatRect & rect, const sf::Color & color)
    {
        sf::Sprite sprite(m_cellSizeTexture);
        sprite.setColor(color);
        util::fitAndCenterInside(sprite, rect);
        m_growFadeSprites.push_back(sprite);
    }

    void Animations::addRisingText(
        const Context & context,
        const std::string & message,
        const sf::Color & color,
        const sf::FloatRect & cellBounds)
    {
        const float heightLimit{ context.layout.window_size_f.y / 20.0f };

        const sf::FloatRect region(
            0.0f, (cellBounds.top - heightLimit), context.layout.window_size_f.x, heightLimit);

        sf::Text text(message, context.media.font(), 99);
        text.setFillColor(color);
        util::fitAndCenterInside(text, region);

        m_growFadeTexts.push_back(text);
    }

    void Animations::cleanup()
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

    void Animations::setupCellTexture()
    {
        sf::Image image;
        image.create(1, 1, sf::Color::White);
        m_cellSizeTexture.loadFromImage(image);
    }

} // namespace snake
