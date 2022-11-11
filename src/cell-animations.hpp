#ifndef SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
#define SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
//
// cell-animations.hpp
//
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
        Animations();

        // prevent all copy and assignment
        Animations(const Animations &) = delete;
        Animations(Animations &&) = delete;
        //
        Animations & operator=(const Animations &) = delete;
        Animations & operator=(Animations &&) = delete;

        void reset();
        void update(Context & context, const float elapsedTimeSec);
        void draw(sf::RenderTarget & target, sf::RenderStates states) const override; //-V813
        void addGrowFadeAnim(const sf::FloatRect & rect, const sf::Color & color);

        void addRisingText(
            const Context & context,
            const std::string & message,
            const sf::Color & color,
            const sf::FloatRect & cellBounds);

        void cleanup();

      private:
        void setupCellTexture();

      private:
        sf::Texture m_cellSizeTexture;
        std::vector<sf::Sprite> m_growFadeSprites;
        std::vector<sf::Text> m_growFadeTexts;
    };
} // namespace snake

#endif // #define SNAKE_CELL_ANIMATIONS_HPP_INCLUDED
