#ifndef CANNONS_ANIMATION_HPP_INCLUDED
#define CANNONS_ANIMATION_HPP_INCLUDED
//
// animation-player.hpp
//
#include "random.hpp"

#include <filesystem>
#include <memory>
#include <tuple>
#include <vector>

#include <SFML/Graphics.hpp>

namespace util
{
    struct AnimConfig
    {
        AnimConfig()
            : AnimConfig(2.0f)
        {
            is_default = true;
        }

        explicit AnimConfig(
            const float durationSec,
            const sf::Color & colorParam = sf::Color::White,
            const sf::BlendMode & blendModeParam = sf::BlendAlpha)
            : is_default(false)
            , duration_sec(durationSec)
            , color(colorParam)
            , blend_mode(blendModeParam)
        {}

        bool is_default;
        float duration_sec;
        sf::Color color;
        sf::BlendMode blend_mode;
    };

    // Animations are simply directories with one or more images that each contain one or more
    // frames.  The images must be named so that simple alpha-numeric sorting works.  Supported
    // image file types are: bmp, jpg, png, and tga.  Directorty names must be formatted
    // this way:   <name>-<width>x<height>
    //
    //  seizure-inducing-flashes-256x512
    //
    // The last number can be skipped if it's the same.
    //  immorally-suggestive-cartoon-128x
    //
    // If multiple animations directories start with the same name...
    //  addictive-carcinogenic-product-advertisement-for-children-100x
    //  addictive-carcinogenic-product-advertisement-for-teens-100x
    //  addictive-carcinogenic-product-advertisement-for-liberals-100x
    //  addictive-carcinogenic-product-advertisement-for-conservatives-100x
    //  addictive-carcinogenic-product-advertisement-for-seniors-100x
    //
    // ...then one call to play("addictive-carcinogenic-product-advertisement")
    // will pick one for you at random.  You know, if you can't decide since their all legal.
    //
    class AnimationPlayer : public sf::Drawable
    {
        struct ParsedDirectoryName
        {
            std::string name;
            sf::Vector2i frame_size;
        };

        struct Image
        {
            std::string filename;
            sf::Texture texture;
            std::vector<sf::IntRect> rects;
        };

        struct ImageCache
        {
            ImageCache(
                const std::size_t indexParam,
                const std::string & nameParam,
                const AnimConfig & configParam,
                const sf::Vector2f & sizeParam)
                : config(configParam)
                , index(indexParam)
                , frame_size(sizeParam)
                , animation_name(nameParam)
                , frame_count(0)
                , images()
            {}

            std::string toString() const;

            AnimConfig config;
            std::size_t index;
            sf::Vector2f frame_size;
            std::string animation_name;
            std::size_t frame_count;
            std::vector<Image> images;
        };

        struct Animation
        {
            AnimConfig config;
            bool is_playing = false;
            sf::Sprite sprite;
            std::size_t cache_index = 0;
            std::size_t frame_index = 0;
            float sec_elapsed = 0.0f;
        };

      public:
        explicit AnimationPlayer(const Random & random, const std::string & pathStr = {});

        // prevent all copy and assignment
        AnimationPlayer(const AnimationPlayer &) = delete;
        AnimationPlayer(AnimationPlayer &&) = delete;
        //
        AnimationPlayer & operator=(const AnimationPlayer &) = delete;
        AnimationPlayer & operator=(AnimationPlayer &&) = delete;

        void reset(const std::string & pathStr = {});

        std::string mediaPath() const { return m_pathStr; }
        void mediaPath(const std::string & pathStr) { m_pathStr = pathStr; }

        std::size_t playingAtOnceMax() const { return m_maxPlayingAtOnceCount; }
        void playingAtOnceMax(const std::size_t maxCount) { m_maxPlayingAtOnceCount = maxCount; }

        bool loadAll(const AnimConfig & config = {});
        bool load(const std::initializer_list<std::string> & names, const AnimConfig & config = {});
        bool load(const std::string & name, const AnimConfig & config = {});

        void configure(const std::string & name, const AnimConfig & config);

        void play(
            const std::string & name, const sf::FloatRect & bounds, const AnimConfig & config = {});

        inline void play(
            const std::string & name,
            const sf::Vector2f & pos,
            const float size,
            const AnimConfig & config = {})
        {
            play(name, sf::FloatRect(pos, sf::Vector2f(size, size)), config);
        }

        void stop(const std::string & name);

        void update(const float elapsedTimeSec);

        void draw(sf::RenderTarget & target, sf::RenderStates states) const override;

        void stopAll();

      private:
        void loadAnimationDirectories(
            const std::string & nameToLoad = "", const AnimConfig & config = {});

        bool willLoadAnimationDirectory(
            const std::filesystem::directory_entry & dirEntry,
            ParsedDirectoryName & parse,
            const std::string & nameToLoad = "") const;

        void loadAnimationDirectory(
            const std::filesystem::directory_entry & dirEntry,
            const ParsedDirectoryName & parse,
            const AnimConfig & config);

        //

        bool loadAnimationImages(
            const std::filesystem::directory_entry & dirEntry, ImageCache & cache) const;

        bool willLoadAnimationImage(const std::filesystem::directory_entry & fileEntry) const;

        bool loadAnimationImage(
            const std::filesystem::directory_entry & fileEntry, ImageCache & cache) const;

        //

        ParsedDirectoryName parseDirectoryName(const std::string & name) const;

        Animation & getAvailableAnimation();

        void updateAnimation(Animation & anim, const float elapsedTimeSec) const;

        void setAnimationFrame(Animation & anim, const std::size_t newFrameIndex) const;

        std::vector<std::size_t> findCacheIndexesByName(const std::string & name) const;

        void createAnimation(
            const std::vector<std::size_t> & possibleCacheIndexes,
            const sf::FloatRect & bounds,
            const AnimConfig & config);

      private:
        const Random & m_random;
        std::string m_pathStr;
        std::vector<Animation> m_animations;
        std::vector<std::unique_ptr<ImageCache>> m_imageCaches;
        std::string m_fileExtensions;
        std::size_t m_maxPlayingAtOnceCount;
    };
} // namespace util

#endif // CANNONS_ANIMATION_HPP_INCLUDED