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
            m_candleTexture = sf::Texture();
            m_starTexture = sf::Texture();

            const std::filesystem::path fontPath{
                path / "font/bpdots-unicase-square/bpdots-unicase-square.otf"
            };

            M_CHECK_SS(m_font.loadFromFile(fontPath.string()), fontPath);

            const std::filesystem::path candleImagePath{ path / "image/puff.png" };
            M_CHECK_SS(m_candleTexture.loadFromFile(candleImagePath.string()), candleImagePath);

            const std::filesystem::path starImagePath{ path / "image/star-8x-full.png" };
            M_CHECK_SS(m_starTexture.loadFromFile(starImagePath.string()), starImagePath);
        }

        const sf::Texture & candleTexture() const { return m_candleTexture; }
        const sf::Texture & starTexture() const { return m_starTexture; }

        const sf::Font & font() const { return m_font; }

      private:
        sf::Font m_font;
        sf::Texture m_candleTexture;
        sf::Texture m_starTexture;
    };
} // namespace snake

#endif // #define SNAKE_MEDIA_HPP_INCLUDED
