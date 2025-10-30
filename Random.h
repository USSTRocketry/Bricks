#pragma once

#include <concepts>
#include <limits>
#include <random>
#include <string>
#include <string_view>

namespace ra
{
/**
 * @brief Singleton class for generating random values.
 *
 * Provides random numbers, vectors, bytes, and strings with customizable ranges
 * or character sets. Includes predefined character sets via RNG::CharSet.
 */
class RNG
{
public:
    /**
     * @brief Predefined character sets for random string generation.
     *
     * Can be combined with operator+ to form custom sets.
     */
    struct CharSet
    {
        static inline const std::string Numeric      = "0123456789";
        static inline const std::string AlphaLower   = "abcdefghijklmnopqrstuvwxyz";
        static inline const std::string AlphaUpper   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static inline const std::string Alpha        = AlphaLower + AlphaUpper;
        static inline const std::string AlphaNumeric = Numeric + Alpha;
        static inline const std::string Hex          = Numeric + "ABCDEF";
        static inline const std::string Special      = "!@#$%^&*()-_=+[]{}|;:',.<>/?~`";
    };

    static RNG& Instance()
    {
        static RNG Inst;
        return Inst;
    }
    /**
     * @brief Generate a single random value in [Min, Max].
     */
    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    T Value(T Min = std::numeric_limits<T>::min(), T Max = std::numeric_limits<T>::max())
    {
        using DistType =
            std::conditional_t<std::integral<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

        DistType dist(Min, Max);
        return dist(Engine);
    }
    template <>
    uint8_t Value<uint8_t>(uint8_t Min, uint8_t Max)
    {
        std::uniform_int_distribution<unsigned int> dist(Min, Max);
        return static_cast<uint8_t>(dist(Engine));
    }
    template <>
    int8_t Value<int8_t>(int8_t Min, int8_t Max)
    {
        std::uniform_int_distribution<int> dist(Min, Max);
        return static_cast<int8_t>(dist(Engine));
    }

    /**
     * @brief Generate a vector of N random values in [Min, Max].
     */
    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    std::vector<T> Vector(size_t N, T Min = std::numeric_limits<T>::min(), T Max = std::numeric_limits<T>::max())
    {
        std::vector<T> Result;
        Result.reserve(N);

        for (size_t i = 0; i < N; ++i)
        {
            Result.push_back(Value(Min, Max));
        }

        return Result;
    }
    /**
     * @brief Generate a vector of N random bytes.
     */
    std::vector<std::byte> ByteVector(size_t N)
    {
        std::vector<std::byte> Result;
        Result.reserve(N);

        for (size_t i = 0; i < N; ++i)
        {
            Result.push_back(static_cast<std::byte>(Value<uint8_t>()));
        }
        return Result;
    }
    /**
     * @brief Generate a random string of given length from the provided character set.
     */
    std::string String(size_t Length, std::string_view Set = CharSet::AlphaNumeric)
    {
        if (Set.empty()) { return {}; }

        std::string Result;
        Result.reserve(Length);

        for (size_t i = 0; i < Length; ++i)
        {
            size_t Index = Value<size_t>(0, Set.size() - 1);
            Result += Set[Index];
        }

        return Result;
    }

private:
    RNG() : Engine(std::random_device {}()) {}
    RNG(const RNG&)            = delete;
    RNG(RNG&&)                 = delete;
    RNG& operator=(const RNG&) = delete;
    RNG& operator=(RNG&&)      = delete;

private:
    std::mt19937 Engine;
};
} // namespace ra
