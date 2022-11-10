// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// cell-animations.cpp
//
#include "cell-animations.hpp"

#include "context.hpp"
#include "layout.hpp"
#include "media.hpp"

//

namespace snake
{
    //

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
} // namespace snake
