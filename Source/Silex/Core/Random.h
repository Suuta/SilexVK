
#pragma once

#include "Core/CoreType.h"
#include <random>


namespace Silex
{
    template<class T>
    concept Randomable = std::is_integral_v<T> || std::is_floating_point_v<T>;
    

    template<Randomable T>
    class Random
    {
    public:

        static T Rand()
        {
            if constexpr (std::is_integral_v<T>)
            {
                std::uniform_int_distribution<T> distribution;
                return distribution(engine);
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> distribution;
                return distribution(engine);
            }
        }

        static T Range(T min, T max)
        {
            if constexpr (std::is_integral_v<T>)
            {
                std::uniform_int_distribution<T> distribution(min, max);
                return distribution(engine);
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> distribution(min, max);
                return distribution(engine);
            }
        }

    private:

        static std::random_device randomDevice;
        static std::mt19937_64    engine;
    };

    template <Randomable T>
    std::random_device Random<T>::randomDevice;

    template <Randomable T>
    std::mt19937_64 Random<T>::engine(Random<T>::randomDevice());
}