#ifndef SNAKE_STATUS_REGION_HPP_INCLUDED
#define SNAKE_STATUS_REGION_HPP_INCLUDED
//
// status-region.hpp
//
#include "check-macros.hpp"
#include "context.hpp"
#include "keys.hpp"
#include "util.hpp"

#include <string>

#include <SFML/Graphics.hpp>

namespace snake
{
    struct Context;
    class GameConfig;
    class Media;

    //
    struct IRegion
    {
        virtual ~IRegion() = default;
        virtual void reset(const Context & context) = 0;
        virtual const sf::FloatRect bounds() const = 0;
        virtual void updateText(const Context & context) = 0;
        virtual void update(Context &, const float elapsedSec) = 0;
        virtual void handleEvent(Context & context, const sf::Event & event) = 0;
        virtual void draw(const Context &, sf::RenderTarget &, sf::RenderStates) const = 0;
    };

    //

    class StatusText : public sf::Drawable
    {
      public:
        StatusText() = default;

        StatusText(
            const Context & context,
            const std::string & prefix,
            const std::size_t digitCount,
            const sf::Color & color,
            const float height);

        void setup(
            const Context & context,
            const std::string & prefix,
            const std::size_t digitCount,
            const sf::Color & color,
            const float height);

        template <typename T>
        void updateNumber(const T number)
        {
            static_assert(std::is_integral_v<T>);

            if (number < T(0))
            {
                updateNumber(T(0));
            }

            std::string numberStr{ std::to_string(number) };
            if (numberStr.length() < m_digitCount)
            {
                numberStr.insert(0, (m_digitCount - numberStr.length()), '0');
            }

            if ((m_digitCount > 3) && ((m_digitCount % 3) == 0))
            {
                std::string temp{ numberStr };
                std::reverse(std::begin(temp), std::end(temp));

                std::size_t i(3);
                while (i < temp.size())
                {
                    temp.insert(i, ",");
                    i += 4;
                }
                std::reverse(std::begin(temp), std::end(temp));
                numberStr = std::move(temp);
            }

            std::string finalStr;
            finalStr.reserve(16);

            finalStr += m_prefix;
            finalStr += ":";
            finalStr += numberStr;

            updateText(finalStr);
        }

        void updateText(const std::string & str);

        sf::FloatRect bounds() const;
        std::string string() const;
        void move(const float horiz, const float vert);
        float height() const { return m_height; }
        void height(const float newHeight);
        sf::Vector2f position() const { return m_text.getPosition(); }
        void position(const float left, const float top) { m_text.setPosition(left, top); }
        void draw(sf::RenderTarget &, sf::RenderStates) const override;
        sf::Vector2f scale() const { return m_text.getScale(); }

      private:
        sf::Text m_text;
        std::string m_prefix;
        float m_height{ 0.0f };
        std::size_t m_digitCount{ 0 };

        static inline const float m_condensedRatio{ 0.7f };
    };

    //

    class StatusRegion : public IRegion
    {
      public:
        StatusRegion() = default;
        virtual ~StatusRegion() override = default;

        void reset(const Context & context) override;

        const sf::FloatRect bounds() const override { return m_statusBounds; }
        const sf::FloatRect textBounds() const { return m_textBounds; }
        void update(Context &, const float) override {}
        void handleEvent(Context &, const sf::Event &) override {}
        void draw(const Context &, sf::RenderTarget &, sf::RenderStates) const override;

        void updateText(const Context & context) override;

      private:
        sf::FloatRect m_statusBounds;
        sf::FloatRect m_textBounds;
        std::vector<StatusText> m_texts;
        sf::Text m_fps;
    };
} // namespace snake

#endif // SNAKE_STATUS_REGION_HPP_INCLUDED
