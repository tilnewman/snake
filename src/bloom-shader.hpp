#ifndef BLOOM_SHADER_HPP_INCLUDED
#define BLOOM_SHADER_HPP_INCLUDED
//
// bloom_shader.hpp
//
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include <SFML/Graphics.hpp>

namespace util
{
    class FullPassFragmentShader
    {
      protected:
        FullPassFragmentShader(const std::string & fragmentShader)
            : m_shader()
            , m_verts(sf::TrianglesStrip, 4)
            , m_renderStates()
        {
            m_shader.loadFromMemory(m_fullPassVertexShaderCode, fragmentShader);

            // sf::VertexArray and sf::Vertex zero initializes so only need to set non-zeros here
            m_verts[0].texCoords.y = 1.0f;
            m_verts[1].texCoords = { 1.0f, 1.0f };
            m_verts[3].texCoords.x = 1.0f;

            m_renderStates.shader = &m_shader;
            m_renderStates.blendMode = sf::BlendNone;
        }

        template <typename... Args_t>
        void setUniform(const std::string & name, const Args_t &... args)
        {
            m_shader.setUniform(name, args...);
        }

        void draw(sf::RenderTarget & target) const
        {
            const sf::Vector2f size(target.getSize());

            // these are the only values that can change
            m_verts[1].position.x = size.x;
            m_verts[2].position.y = size.y;
            m_verts[3].position = size;

            // no need to clear before draw because this is "full pass" and sf::BlendNone
            target.draw(m_verts, m_renderStates);
        }

      private:
        sf::Shader m_shader;
        mutable sf::VertexArray m_verts;
        sf::RenderStates m_renderStates;

        static inline const std::string m_fullPassVertexShaderCode{ "\
void main()\
{\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
    gl_TexCoord[0] = gl_MultiTexCoord0;\
}" };
    };

    //

    struct BrightFilterShader : public FullPassFragmentShader
    {
        BrightFilterShader()
            : FullPassFragmentShader(m_fragmentShaderCode)
        {}

        void apply(const sf::RenderTexture & input, sf::RenderTexture & output)
        {
            setUniform("source", input.getTexture());
            draw(output);
            output.display();
        }

        static inline const std::string m_fragmentShaderCode{ "\
uniform sampler2D source;\
\
const float Threshold = 0.7;\
const float Factor = 4.0;\
\
void main()\
{\
    vec4 sourceFragment = texture2D(source, gl_TexCoord[0].xy);\
    float luminance = sourceFragment.r * 0.2126 + sourceFragment.g * 0.7152 + sourceFragment.b *0.0722;\
    sourceFragment *= clamp(luminance - Threshold, 0.0, 1.0) * Factor;\
    gl_FragColor = sourceFragment;\
}" };
    };

    //

    struct AddShader : public FullPassFragmentShader
    {
        AddShader()
            : FullPassFragmentShader(m_fragmentShaderCode)
        {}

        void apply(
            const sf::RenderTexture & source,
            const sf::RenderTexture & bloom,
            sf::RenderTarget & output)
        {
            setUniform("source", source.getTexture());
            setUniform("bloom", bloom.getTexture());
            draw(output);
        }

        static inline const std::string m_fragmentShaderCode{ "\
uniform sampler2D source;\
uniform sampler2D bloom;\
\
void main()\
{\
    vec4 sourceFragment = texture2D(source, gl_TexCoord[0].xy);\
    vec4 bloomFragment = texture2D(bloom, gl_TexCoord[0].xy);\
    gl_FragColor = sourceFragment + bloomFragment;\
}" };
    };

    //

    struct DownSampleShader : public FullPassFragmentShader
    {
        DownSampleShader()
            : FullPassFragmentShader(m_fragmentShaderCode)
        {}

        void apply(const sf::RenderTexture & input, sf::RenderTexture & output)
        {
            setUniform("source", input.getTexture());
            setUniform("sourceSize", sf::Vector2f(input.getSize()));
            draw(output);
            output.display();
        }

        static inline const std::string m_fragmentShaderCode{ "\
uniform sampler2D 	source;\
uniform vec2 		sourceSize;\
\
void main()\
{\
    vec2 pixelSize = vec2(1.0 / sourceSize.x, 1.0 / sourceSize.y);\
    vec2 textureCoordinates = gl_TexCoord[0].xy;\
    vec4 color = texture2D(source, textureCoordinates);\
    color += texture2D(source, textureCoordinates + vec2(1.0,  0.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(-1.0,  0.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(0.0,  1.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(0.0, -1.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(1.0,  1.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(-1.0, -1.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(1.0, -1.0) * pixelSize);\
    color += texture2D(source, textureCoordinates + vec2(-1.0,  1.0) * pixelSize);\
    gl_FragColor = color / 9.0;\
}" };
    };

    //

    struct BlurShader : public FullPassFragmentShader
    {
        explicit BlurShader(const std::size_t multiPassCount = 2)
            : FullPassFragmentShader(m_fragmentShaderCode)
            , m_multiPassCount(multiPassCount)
        {}

        std::size_t multiPassCount() const { return m_multiPassCount; }
        void multiPassCount(const std::size_t count) { m_multiPassCount = count; }

        void apply(
            const sf::RenderTexture & input,
            sf::RenderTexture & output,
            const sf::Vector2f & offsetFactor)
        {
            setUniform("source", input.getTexture());
            setUniform("offsetFactor", offsetFactor);
            draw(output);
            output.display();
        }

        // TODO skip if m_multiPassCount is zero?
        void applyMultiPass(sf::RenderTexture & left, sf::RenderTexture & right)
        {
            const sf::Vector2f size(left.getSize());

            sf::Vector2f offsetFactorVert(0.0f, (1.0f / size.y));
            sf::Vector2f offsetFactorHoriz((1.0f / size.x), 0.0f);

            const float blurMultiplier(1.25f);
            for (std::size_t count(0); count < m_multiPassCount; ++count)
            {
                apply(left, right, offsetFactorVert);
                apply(right, left, offsetFactorHoriz);

                offsetFactorVert *= blurMultiplier;
                offsetFactorHoriz *= blurMultiplier;
            }
        }

      private:
        std::size_t m_multiPassCount;

        // this is actually a Gaussian Blur
        static inline const std::string m_fragmentShaderCode{ "\
uniform sampler2D 	source;\
uniform vec2 		offsetFactor;\
\
void main()\
{\
    vec2 textureCoordinates = gl_TexCoord[0].xy;\
    vec4 color = vec4(0.0);\
    color += texture2D(source, textureCoordinates - 4.0 * offsetFactor) * 0.0162162162;\
    color += texture2D(source, textureCoordinates - 3.0 * offsetFactor) * 0.0540540541;\
    color += texture2D(source, textureCoordinates - 2.0 * offsetFactor) * 0.1216216216;\
    color += texture2D(source, textureCoordinates - offsetFactor) * 0.1945945946;\
    color += texture2D(source, textureCoordinates) * 0.2270270270;\
    color += texture2D(source, textureCoordinates + offsetFactor) * 0.1945945946;\
    color += texture2D(source, textureCoordinates + 2.0 * offsetFactor) * 0.1216216216;\
    color += texture2D(source, textureCoordinates + 3.0 * offsetFactor) * 0.0540540541;\
    color += texture2D(source, textureCoordinates + 4.0 * offsetFactor) * 0.0162162162;\
    gl_FragColor = color;\
}" };
    };

    //

    class BloomEffect
    {
        typedef std::array<sf::RenderTexture, 2> RenderTextureArray;

      public:
        BloomEffect()
            : m_addShader()
            , m_blurShader()
            , m_downSampleShader()
            , m_brightFilterShader()
            , m_brightnessTexture()
            , m_halfSizeTextures()
            , m_quarterSizeTextures()
        {}

        static bool isSupported() { return sf::Shader::isAvailable(); }

        // don't forget to call output.display() after this function!
        void apply(const sf::RenderTexture & input, sf::RenderTarget & output)
        {
            setupRenderTextures(input.getSize());

            m_brightFilterShader.apply(input, m_brightnessTexture);

            m_downSampleShader.apply(m_brightnessTexture, m_halfSizeTextures[0]);
            m_blurShader.applyMultiPass(m_halfSizeTextures[0], m_halfSizeTextures[1]);

            m_downSampleShader.apply(m_halfSizeTextures[0], m_quarterSizeTextures[0]);
            m_blurShader.applyMultiPass(m_quarterSizeTextures[0], m_quarterSizeTextures[1]);

            m_addShader.apply(
                m_halfSizeTextures[0], m_quarterSizeTextures[0], m_halfSizeTextures[1]);
            m_halfSizeTextures[1].display();

            m_addShader.apply(input, m_halfSizeTextures[1], output);
        }

        std::size_t blurMultiPassCount() const { return m_blurShader.multiPassCount(); }
        void blurMultiPassCount(const std::size_t count) { m_blurShader.multiPassCount(count); }

      private:
        // calling this every time could thrash sf::RenderTextures.  You have been warned.
        void setupRenderTextures(const sf::Vector2u & size)
        {
            createRenderTexture(m_brightnessTexture, size);

            const sf::Vector2u halfSize((size.x / 2), (size.y / 2));
            createRenderTexture(m_halfSizeTextures[0], halfSize);
            createRenderTexture(m_halfSizeTextures[1], halfSize);

            const sf::Vector2u quarterSize((size.x / 4), (size.y / 4));
            createRenderTexture(m_quarterSizeTextures[0], quarterSize);
            createRenderTexture(m_quarterSizeTextures[1], quarterSize);
        }

        void createRenderTexture(sf::RenderTexture & renderTexture, const sf::Vector2u & size)
        {
            if (renderTexture.getSize() == size)
            {
                return;
            }

            renderTexture.create(size.x, size.y);
            renderTexture.setSmooth(true);
        }

      private:
        AddShader m_addShader;
        BlurShader m_blurShader;
        DownSampleShader m_downSampleShader;
        BrightFilterShader m_brightFilterShader;

        sf::RenderTexture m_brightnessTexture;
        RenderTextureArray m_halfSizeTextures;
        RenderTextureArray m_quarterSizeTextures;
    };

    //

    class BloomEffectHelper
    {
      public:
        explicit BloomEffectHelper(sf::RenderWindow & window)
            : m_isEnabled(false)
            , m_bloomEffect()
            , m_window(window)
            , m_sideTexture()
        {
            if (!m_bloomEffect.isSupported())
            {
                throw std::runtime_error("BloomEffectHelper's constructor just found out that sfml "
                                         "shaders are not supported on your shitty video card.");
            }

            if (!m_sideTexture.create(window.getSize().x, window.getSize().y)) //-V807
            {
                throw std::runtime_error(
                    "BloomEffectHelper's constructor was unable to create a side sf::RenderTexture "
                    "of size " +
                    std::to_string(window.getSize().x) + "x" + std::to_string(window.getSize().y));
            }
        }

        // prevent all copy and assignment
        BloomEffectHelper(const BloomEffectHelper &) = delete;
        BloomEffectHelper(BloomEffectHelper &&) = delete;
        //
        BloomEffectHelper & operator=(const BloomEffectHelper &) = delete;
        BloomEffectHelper & operator=(BloomEffectHelper &&) = delete;

        bool isOpen() const { return m_window.isOpen(); }

        void close()
        {
            m_window.close();
            isEnabled(false);
        }

        bool isEnabled() const { return (m_isEnabled && isOpen()); }
        void isEnabled(const bool willEnable) { m_isEnabled = (willEnable && isOpen()); }

        std::size_t blurMultipassCount() const { return m_bloomEffect.blurMultiPassCount(); }

        void blurMultipassCount(const std::size_t newCount)
        {
            m_bloomEffect.blurMultiPassCount(newCount);
        }

        void clear(const sf::Color & color = sf::Color::Black) { renderTarget().clear(color); }

        void draw(sf::Drawable & toDraw, const sf::RenderStates & states = {})
        {
            renderTarget().draw(toDraw, states);
        }

        void display()
        {
            if (m_isEnabled)
            {
                m_sideTexture.display();
                m_bloomEffect.apply(m_sideTexture, m_window);
            }

            m_window.display();
        }

        sf::RenderTarget & renderTarget()
        {
            if (m_isEnabled)
            {
                return m_sideTexture;
            }
            else
            {
                return m_window;
            }
        }

      private:
        bool m_isEnabled;
        BloomEffect m_bloomEffect;
        sf::RenderWindow & m_window;
        sf::RenderTexture m_sideTexture;
    };
} // namespace util

#endif // BLOOM_SHADER_HPP_INCLUDED
