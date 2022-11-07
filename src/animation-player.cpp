// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// animation-player.cpp
//
#include "animation-player.hpp"
#include "util.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace util
{
    AnimationPlayer::AnimationPlayer(const Random & random, const std::string & pathStr)
        : m_random(random)
        , m_pathStr(pathStr)
        , m_animations()
        , m_imageCaches()
        , m_fileExtensions(".bmp/.jpg/.jpeg/.png/.tga")
        , m_maxPlayingAtOnceCount(100)
    { }

    void AnimationPlayer::stop(const std::string & name)
    {
        const std::vector<std::size_t> indexes { findCacheIndexesByName(name) };

        for (Animation & anim : m_animations)
        {
            if (std::find(std::begin(indexes), std::end(indexes), anim.cache_index)
                != std::end(indexes))
            {
                anim.is_playing = false;
            }
        }
    }

    void AnimationPlayer::reset(const std::string & newPathStr)
    {
        if (!newPathStr.empty())
        {
            m_pathStr = newPathStr;
        }

        stopAll();
        m_animations.clear();
        m_imageCaches.clear();
    }

    bool AnimationPlayer::loadAll(const AnimConfig & config)
    {
        reset();
        loadAnimationDirectories("", config);
        return !m_imageCaches.empty();
    }

    bool AnimationPlayer::load(
        const std::initializer_list<std::string> & names, const AnimConfig & config)
    {
        bool success { true };

        for (const std::string & name : names)
        {
            if (!load(name, config))
            {
                success = false;
            }
        }

        return success;
    }

    bool AnimationPlayer::load(const std::string & name, const AnimConfig & config)
    {
        if (name.empty())
        {
            return false;
        }

        stop(name);

        loadAnimationDirectories(name, config);

        return !findCacheIndexesByName(name).empty();
    }

    void AnimationPlayer::configure(const std::string & name, const AnimConfig & config)
    {
        for (const std::size_t index : findCacheIndexesByName(name))
        {
            m_imageCaches.at(index)->config = config;
        }
    }

    void AnimationPlayer::stopAll()
    {
        for (Animation & anim : m_animations)
        {
            anim.is_playing = false;
        }
    }

    void AnimationPlayer::play(
        const std::string & name, const sf::FloatRect & bounds, const AnimConfig & config)
    {
        if (name.empty())
        {
            std::cerr << "AnimationPlayer::play() called with an empty name." << std::endl;
            return;
        }

        if ((bounds.width < 1.0f) || (bounds.height < 1.0f))
        {
            std::cerr << "AnimationPlayer::play(bounds=" << bounds
                      << ") called with bounds of sizes less than one." << std::endl;
            return;
        }

        std::vector<std::size_t> nameMatchingIndexes(findCacheIndexesByName(name));
        if (nameMatchingIndexes.empty())
        {
            std::cout << "AnimationPlayer::play(\"" << name
                      << "\") called, but none had that name...";

            if (!load(name, config))
            {
                std::cout << "AND none were found to load either.  So nothing will happen."
                          << std::endl;

                return;
            }

            nameMatchingIndexes = findCacheIndexesByName(name);
            if (nameMatchingIndexes.empty())
            {
                std::cout << "AND even though some anims with that name were loaded, something "
                             "else went wrong away.  Go figure.  So nothing will happen."
                          << std::endl;

                return;
            }

            std::cout << "BUT was able to find and load it.  So it's gonna play now." << std::endl;
        }

        createAnimation(nameMatchingIndexes, bounds, config);
    }

    void AnimationPlayer::update(const float elapsedTimeSec)
    {
        for (Animation & anim : m_animations)
        {
            updateAnimation(anim, elapsedTimeSec);
        }
    }

    void AnimationPlayer::draw(sf::RenderTarget & target, sf::RenderStates states) const
    {
        for (const Animation & anim : m_animations)
        {
            if (anim.is_playing)
            {
                const auto blendModeOrig { states.blendMode };
                states.blendMode = anim.config.blend_mode;
                target.draw(anim.sprite, states);
                states.blendMode = blendModeOrig;
            }
        }
    }

    void AnimationPlayer::loadAnimationDirectories(
        const std::string & nameToLoad, const AnimConfig & config)
    {
        std::filesystem::path path(m_pathStr);
        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
        {
            path = std::filesystem::current_path();
        }

        std::filesystem::recursive_directory_iterator dirIter(
            path, std::filesystem::directory_options::skip_permission_denied);

        for (const std::filesystem::directory_entry & entry : dirIter)
        {
            ParsedDirectoryName parse;

            if (willLoadAnimationDirectory(entry, parse, nameToLoad))
            {
                loadAnimationDirectory(entry, parse, config);
            }
        }

        if (m_imageCaches.empty())
        {
            std::cerr
                << "AnimationPlayer Error:  No valid animation directories were found.  Supported "
                   "image file types: "
                << m_fileExtensions << std::endl;
        }
    }

    bool AnimationPlayer::willLoadAnimationDirectory(
        const std::filesystem::directory_entry & dirEntry,
        ParsedDirectoryName & parse,
        const std::string & nameToLoad) const
    {
        parse = parseDirectoryName(dirEntry.path().filename().string());

        const bool isDirNameValid { !parse.name.empty() && (parse.frame_size.x > 0)
                                    && (parse.frame_size.y > 0) };

        if (!isDirNameValid)
        {
            return false;
        }

        if (!nameToLoad.empty())
        {
            const bool dirNameStartsWith { parse.name.find(nameToLoad, 0) == 0 };
            if (dirNameStartsWith)
            {
                return true;
            }

            return false;
        }

        return true;
    }

    void AnimationPlayer::loadAnimationDirectory(
        const std::filesystem::directory_entry & dirEntry,
        const ParsedDirectoryName & parse,
        const AnimConfig & config)
    {
        auto imageCache { std::make_unique<ImageCache>(
            m_imageCaches.size(), parse.name, config, sf::Vector2f(parse.frame_size)) };

        if (loadAnimationImages(dirEntry, *imageCache))
        {
            std::cout << "Loaded Animation: " << imageCache->toString() << std::endl;
            m_imageCaches.push_back(std::move(imageCache));
        }
    }

    bool AnimationPlayer::loadAnimationImages(
        const std::filesystem::directory_entry & dirEntry, ImageCache & cache) const
    {
        std::filesystem::directory_iterator dirIter(
            dirEntry.path(), std::filesystem::directory_options::skip_permission_denied);

        for (const std::filesystem::directory_entry & fileEntry : dirIter)
        {
            if (willLoadAnimationImage(fileEntry))
            {
                if (!loadAnimationImage(fileEntry, cache))
                {
                    return false;
                }
            }
        }

        if (cache.images.empty())
        {
            std::cerr
                << "AnimationPlayer Error:  Found a directory that is named like an animation "
                   "directory here: \""
                << dirEntry.path().string() << "\", but was unable to load any images from it."
                << std::endl;

            return false;
        }

        cache.frame_count = 0;
        for (const Image & image : cache.images)
        {
            cache.frame_count += image.rects.size();
        }

        if (0 == cache.frame_count)
        {
            std::cerr
                << "AnimationPlayer Error:  Found a directory that is named like an animation "
                   "directory: \""
                << dirEntry.path().string()
                << "\", and was unable to load images from it, but somehow the frame count was "
                   "still zero."
                << std::endl;

            return false;
        }

        // directory iterators do not always go in alphanumeric order, so sort here just in case
        std::sort(
            std::begin(cache.images),
            std::end(cache.images),
            [](const Image & left, const Image & right) {
                return (left.filename < right.filename);
            });

        return true;
    }

    bool AnimationPlayer::willLoadAnimationImage(
        const std::filesystem::directory_entry & fileEntry) const
    {
        if (!fileEntry.is_regular_file())
        {
            return false;
        }

        const std::string fileName(fileEntry.path().filename().string());
        if (fileName.empty())
        {
            return false;
        }

        const std::string fileExt(fileEntry.path().filename().extension().string());
        if ((fileExt.size() != 4) && (fileExt.size() != 5))
        {
            return false;
        }

        return (m_fileExtensions.find(fileExt) < m_fileExtensions.size());
    }

    bool AnimationPlayer::loadAnimationImage(
        const std::filesystem::directory_entry & fileEntry, ImageCache & cache) const
    {
        const sf::Vector2i frameSize(cache.frame_size);

        Image image;

        if (!image.texture.loadFromFile(fileEntry.path().string()))
        {
            std::cerr << "AnimationPlayer Error:  Found a supported file: \""
                      << fileEntry.path().string() << "\", but an error occurred while loading it."
                      << std::endl;

            return false;
        }

        image.texture.setSmooth(true);

        const sf::Vector2i imageSize(image.texture.getSize());

        for (int vert(0); vert < imageSize.y; vert += frameSize.y)
        {
            for (int horiz(0); horiz < imageSize.x; horiz += frameSize.x)
            {
                image.rects.push_back({ sf::Vector2i(horiz, vert), frameSize });
            }
        }

        if (image.rects.empty())
        {
            std::cerr << "AnimationPlayer Error:  Found a supported file: \""
                      << fileEntry.path().string() << "\", but no frame rects could be established."
                      << std::endl;

            return false;
        }

        cache.images.push_back(std::move(image));

        return true;
    }

    void AnimationPlayer::createAnimation(
        const std::vector<std::size_t> & possibleCacheIndexes,
        const sf::FloatRect & bounds,
        const AnimConfig & configParam)
    {
        const std::size_t randomCacheIndex { m_random.from(possibleCacheIndexes) };
        const ImageCache & cache { *m_imageCaches.at(randomCacheIndex) };
        Animation & anim { getAvailableAnimation() };

        if (configParam.is_default)
        {
            anim.config = cache.config;
        }
        else
        {
            anim.config = configParam;
        }

        anim.cache_index = cache.index;
        anim.frame_index = 0;
        anim.sec_elapsed = 0.0f;
        anim.is_playing = true;

        setAnimationFrame(anim, 0);

        util::fitAndCenterInside(anim.sprite, bounds);
        anim.sprite.setColor(anim.config.color);
    }

    AnimationPlayer::ParsedDirectoryName
        AnimationPlayer::parseDirectoryName(const std::string & name) const
    {
        const auto size(name.size());
        const auto dashIndex(name.rfind('-'));
        const auto xIndex(name.rfind('x'));

        if ((0 == dashIndex) || (dashIndex >= size) || (xIndex >= size) || (xIndex <= dashIndex))
        {
            return {};
        }

        try
        {
            const std::string animName(name.substr(0, dashIndex));
            const std::string widthStr(name.substr((dashIndex + 1), (dashIndex - xIndex - 1)));
            const std::string heightStr(name.substr(xIndex + 1));

            const int width(std::stoi(widthStr));
            const int height((heightStr.empty()) ? width : std::stoi(heightStr)); //-V537

            return { animName, sf::Vector2i(width, height) };
        }
        catch (...)
        { //-V565
        }

        return {};
    }

    AnimationPlayer::Animation & AnimationPlayer::getAvailableAnimation()
    {
        for (Animation & anim : m_animations)
        {
            if (!anim.is_playing)
            {
                return anim;
            }
        }

        if (m_animations.size() > m_maxPlayingAtOnceCount)
        {
            m_animations[0].is_playing = false;
            return m_animations[0];
        }
        else
        {
            return m_animations.emplace_back();
        }
    }

    std::vector<std::size_t> AnimationPlayer::findCacheIndexesByName(const std::string & name) const
    {
        std::vector<std::size_t> indexes;

        for (std::size_t i(0); i < m_imageCaches.size(); ++i)
        {
            const bool animNameStartsWith { m_imageCaches.at(i)->animation_name.find(name, 0)
                                            == 0 };
            if (animNameStartsWith)
            {
                indexes.push_back(i);
            }
        }

        return indexes;
    }

    void AnimationPlayer::updateAnimation(Animation & anim, const float elapsedTimeSec) const
    {
        if (!anim.is_playing)
        {
            return;
        }

        anim.sec_elapsed += elapsedTimeSec;

        const ImageCache & cache { *m_imageCaches.at(anim.cache_index) };
        const float frameCount { static_cast<float>(cache.frame_count) };
        const float durationRatio { (anim.sec_elapsed / anim.config.duration_sec) };
        const std::size_t newFrameIndex(static_cast<std::size_t>(frameCount * durationRatio));

        if (newFrameIndex > cache.frame_count)
        {
            anim.is_playing = false;
            return;
        }

        if (newFrameIndex == anim.frame_index)
        {
            return;
        }

        setAnimationFrame(anim, newFrameIndex);
    }

    // TODO this should not iterate but calculate maybe make a function of ImageCache to jump to
    // whatever frame...
    void AnimationPlayer::setAnimationFrame(Animation & anim, const std::size_t newFrameIndex) const
    {
        anim.frame_index = newFrameIndex;
        const ImageCache & cache { *m_imageCaches.at(anim.cache_index) };

        std::size_t frameCounter { 0 };
        for (const Image & image : cache.images)
        {
            for (const sf::IntRect & rect : image.rects)
            {
                if (newFrameIndex == frameCounter)
                {
                    anim.sprite.setTexture(image.texture);
                    anim.sprite.setTextureRect(rect);
                    return;
                }

                ++frameCounter;
            }
        }
    }

    std::string AnimationPlayer::ImageCache::toString() const
    {
        const std::string pad("  ");

        std::ostringstream ss;

        ss << "#" << index;
        ss << pad;

        ss << std::setw(14) << std::right << animation_name;
        ss << pad;

        ss << std::setw(3) << std::right << static_cast<int>(frame_size.x);
        ss << "x";
        ss << std::setw(3) << std::left << static_cast<int>(frame_size.y);
        ss << pad;

        ss << "x" << frame_count;
        return ss.str();
    }
} // namespace util
