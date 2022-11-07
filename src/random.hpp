#ifndef RANDOM_HPP_INCLUDED
#define RANDOM_HPP_INCLUDED
//
// random.hpp
//
#include <initializer_list>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

namespace util
{
    class Random
    {
    public:
        Random()
            : Random(std::random_device {}())
        { }

        explicit Random(const std::random_device::result_type seed)
            : m_engine()
        {
            std::seed_seq seedSequence { seed };
            m_engine.seed(seedSequence);

            // Warm-up-skipping is good standard practice when working with PRNGs, but the Mersenne
            // Twister is notoriously predictable in the beginning.  This is especially true when
            // you don't provide a good (full sized) seed, which I am not because I want the ease of
            // troubleshooting games. Anything from thousands to hundreds-thousands works fine here.
            m_engine.discard(123456);
        }

        // prevent all copy and assignment
        Random(const Random &) = delete;
        Random(Random &&) = delete;
        //
        Random & operator=(const Random &) = delete;
        Random & operator=(Random &&) = delete;

        template <typename T>
        T fromTo(const T from, const T to) const
        {
            static_assert(std::is_arithmetic_v<T>);

            if (to < from)
            {
                return fromTo(to, from);
            }

            if constexpr (std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> distribution(
                    from, std::nextafter(to, std::numeric_limits<T>::max()));

                return distribution(m_engine);
            }
            else if constexpr (sizeof(T) == 1)
            {
                return static_cast<T>(fromTo<int>(static_cast<int>(from), static_cast<int>(to)));
            }
            else
            {
                std::uniform_int_distribution<T> distribution(from, to);
                return distribution(m_engine);
            }
        }

        template <typename T>
        T zeroTo(const T to) const
        {
            return fromTo(T(0), to);
        }

        template <typename T = float>
        T ratio() const
        {
            return zeroTo(T(1));
        }

        template <typename T>
        std::size_t index(const T size) const
        {
            if (size <= 1)
            {
                return 0;
            }

            return static_cast<std::size_t>(zeroTo(size - T(1)));
        }

        template <typename T>
        std::size_t indexFrom(const T & container) const
        {
            if (container.size() == 0)
            {
                std::string message;
                message += "Random::index<T>(container) but that container.size() == 0!";
                throw std::runtime_error(message);
            }

            return index(container.size());
        }

        inline bool boolean() const { return (zeroTo(1) == 0); }

        template <typename Iter_t>
        auto & from(Iter_t first, const Iter_t last) const
        {
            if (last == first)
            {
                std::string message;
                message += "Random::from<T>(iter,iter) but the container was empty!";
                throw std::runtime_error(message);
            }

            const auto offset { zeroTo(std::distance(first, last) - 1) };
            std::advance(first, static_cast<std::ptrdiff_t>(offset));
            return *first;
        }

        template <typename T>
        auto & from(T & container) const
        {
            return from(std::begin(container), std::end(container));
        }

        template <typename T>
        const T & from(const std::initializer_list<T> & list) const
        {
            return from(std::begin(list), std::end(list));
        }

        template <typename Iter_t>
        void shuffle(const Iter_t first, const Iter_t last) const
        {
            std::shuffle(first, last, m_engine);
        }

        template <typename T>
        void shuffle(T & container) const
        {
            shuffle(std::begin(container), std::end(container));
        }

        enum class Option
        {
            None = 0,
            InvertNormal
        };

        // param = 1.0 will make the classic full bell curve with no values to clamp
        // param > 1.0 will start returning numbers outside of [0,1]
        // param < 1.0 will start reducing the possibility of values at reach either end-point [0,1]
        template <typename T = float>
        T normalRatio(const T stdDevRatio = T(1.0), const Option style = Option::None) const
        {
            static_assert(std::is_floating_point_v<T>);

            // std::normal_distribution might crash if the std-dev is <=0
            if (!(stdDevRatio > T(0.0)))
            {
                return normalRatio(std::numeric_limits<T>::epsilon(), style);
            }

            // The 0.125 comes from (0.5 / 4.0), which means the values will go no farther from the
            // mid-point (0.5) than four standard-eviations from that mid-point in each direction.
            std::normal_distribution<T> distribution(T(0.5), (T(0.125) * stdDevRatio));

            T randNormal { distribution(m_engine) };

            while ((randNormal < 0.0f) || (randNormal > 1.0f))
            {
                randNormal = distribution(m_engine);
            }

            if (style == Option::InvertNormal)
            {
                if (randNormal < T(0.5))
                {
                    randNormal += T(0.5);
                }
                else
                {
                    randNormal -= T(0.5);
                }
            }

            return randNormal;
        }

        template <typename T>
        T normalFromTo(
            const T from,
            const T to,
            const float stdDevRatio = 1.0f,
            const Option style = Option::None) const
        {
            static_assert(std::is_arithmetic_v<T>);

            if (to < from)
            {
                return normalFromTo(to, from, stdDevRatio, style); //-V764
            }

            // std::normal_distribution might crash if the std-dev is <=0
            if (!(stdDevRatio > T(0.0)))
            {
                return normalFromTo(from, to, std::numeric_limits<float>::epsilon(), style);
            }

            const float range { static_cast<float>(to - from) };
            const float ratio { normalRatio<float>(stdDevRatio, style) };
            const T offset { static_cast<T>(range * ratio) };
            return std::clamp((from + offset), from, to);
        }

    private:
        mutable std::mt19937 m_engine;
    };
} // namespace util

#endif // RANDOM_HPP_INCLUDED
