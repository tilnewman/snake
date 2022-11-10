// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// status-region.cpp
//
#include "status-region.hpp"

#include "layout.hpp"
#include "media.hpp"
#include "settings.hpp"

namespace snake
{
    StatusText::StatusText(
        const Context & context,
        const std::string & prefix,
        const std::size_t digitCount,
        const sf::Color & color,
        const float height)
    {
        setup(context, prefix, digitCount, color, height);
    }

    void StatusText::setup(
        const Context & context,
        const std::string & prefix,
        const std::size_t digitCount,
        const sf::Color & color,
        const float height)
    {
        m_prefix = prefix;
        m_digitCount = digitCount;
        m_height = height;

        m_text.setFont(context.media.font());
        m_text.setCharacterSize(99);
        m_text.setFillColor(color);

        updateNumber(0);
    }

    void StatusText::updateText(const std::string & str)
    {
        m_text.setString(str);
        height(m_height);
    }

    sf::FloatRect StatusText::bounds() const { return m_text.getGlobalBounds(); }

    std::string StatusText::string() const { return m_text.getString().toAnsiString(); }

    void StatusText::move(const float horiz, const float vert) { m_text.move(horiz, vert); }

    void StatusText::height(const float newHeight)
    {
        if (!(m_text.getLocalBounds().height > 0.0f))
        {
            return;
        }

        m_height = newHeight;
        const float scale{ m_height / m_text.getLocalBounds().height };
        m_text.setScale((scale * m_condensedRatio), scale);
        util::setOriginToPosition(m_text);
    }

    void StatusText::draw(sf::RenderTarget & target, sf::RenderStates states) const
    {
        target.draw(m_text, states);
    }

    //

    void StatusRegion::reset(const Context & context)
    {
        m_texts.clear();
        m_texts.reserve(10);

        m_statusBounds = context.layout.status_bounds_f;
        m_textBounds = util::scaleRectInPlaceCopy(m_statusBounds, { 0.95f, 0.65f });

        float textHeight{ m_textBounds.height };

        // all kinds of text colors
        const sf::Color yellow(230, 190, 50);
        // const sf::Color yellowOrange(255, 190, 0);
        const sf::Color orange(255, 170, 60);
        // const sf::Color creamSkin(250, 230, 190);
        const sf::Color creamCool(230, 190, 180, 192);

        m_texts.emplace_back(context, "LEVEL", 3, orange, textHeight);
        m_texts.emplace_back(context, "SCORE", 9, yellow, textHeight);
        // m_texts.emplace_back(context, "LEFT", 3, creamSkin, textHeight);
        m_texts.emplace_back(context, "FPS", 3, creamCool, textHeight);

        const float betweenPadCount{ static_cast<float>(m_texts.size() - 1) };
        const float betweenPadMin{ m_statusBounds.height };
        const float betweenPadMinSum{ betweenPadMin * betweenPadCount };
        const float heightDecrement{ 0.005f };

        float highestTopPos{ util::bottom(m_statusBounds) }; // anything taller than this is fine
        float totalLength{ 0.0f };
        do
        {
            totalLength = betweenPadMinSum;

            for (StatusText & statusText : m_texts)
            {
                textHeight -= heightDecrement;

                if (statusText.string().find(',') < statusText.string().size())
                {
                    statusText.height(textHeight * 1.25f);
                }
                else
                {
                    statusText.height(textHeight);
                }

                const float posTop{ (context.layout.board_bounds_f.top -
                                     statusText.bounds().height) +
                                    (m_statusBounds.height / 23.0f) };

                statusText.position(0.0f, posTop);

                totalLength += statusText.bounds().width;

                if (highestTopPos > statusText.bounds().top)
                {
                    highestTopPos = statusText.bounds().top;
                }
            }
        } while (totalLength > m_textBounds.width);

        const float emptyHorizSpace{ m_textBounds.width - totalLength };
        const float betweenPadActual{ betweenPadMin + (emptyHorizSpace / betweenPadCount) };

        float posLeft{ m_textBounds.left };
        for (StatusText & statusText : m_texts)
        {
            statusText.position(posLeft, highestTopPos);

            posLeft += statusText.bounds().width;
            posLeft += betweenPadActual;
        }

        // raise all text a little to make sure commas don't touch the board
        for (StatusText & statusText : m_texts)
        {
            statusText.move(0.0f, -6.0f);
        }
    }

    void StatusRegion::draw(sf::RenderTarget & target, sf::RenderStates states) const
    {
        for (const StatusText & statusText : m_texts)
        {
            target.draw(statusText, states);
        }
    }

    void StatusRegion::updateText(const Context & context)
    {
        // 0:level, 1:score, 2:Remaining, 3:fps
        m_texts.at(0).updateNumber(context.game.level().number);
        m_texts.at(1).updateNumber(context.game.score());
        // m_texts.at(2).updateNumber(context.game.level().remainingToEat());
        m_texts.at(2).updateNumber(context.fps);
    }
} // namespace snake
