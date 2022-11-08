#ifndef SNAKE_MEDIA_HPP_INCLUDED
#define SNAKE_MEDIA_HPP_INCLUDED
//
// media.hpp
//
#include "check-macros.hpp"

#include <filesystem>

#include <SFML/Graphics/Font.hpp>

namespace snake
{
    class Media
    {
      public:
        Media() = default;

        void reset(const std::filesystem::path & path)
        {
            m_font = sf::Font();

            const std::filesystem::path fontPath{
                path / "font/bpdots-unicase-square/bpdots-unicase-square.otf"
            };

            M_CHECK_SS(m_font.loadFromFile(fontPath.string()), fontPath);
        }

        const sf::Font & font() const { return m_font; }

      private:
        sf::Font m_font;
    };
} // namespace snake

#endif // #define SNAKE_MEDIA_HPP_INCLUDED
