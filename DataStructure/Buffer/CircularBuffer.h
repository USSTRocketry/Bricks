#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace RA::Bricks
{
template <typename T, uint32_t BuffSize = 64>
class CircularBuffer
{
public:
    /*
     * Queues item to the buffer, does not overwrite elements
     * @returns true if queued
     */
    bool Queue(std::same_as<T> auto&& Item)
    {
        if (m_Size == BuffSize) { return false; }

        m_Buffer[m_Tail] = std::forward<decltype(Item)>(Item);
        m_Tail           = (m_Tail + 1) % BuffSize;
        m_Size++;

        return true;
    }
    /*
     * Get item from the buffer
     */
    std::optional<T> Dequeue()
    {
        if (m_Size == 0) { return std::nullopt; }

        T Item = std::move(m_Buffer[m_Head]);
        m_Head = (m_Head + 1) % BuffSize;
        m_Size--;

        return {std::move(Item)};
    }

    // current elements in buffer
    uint32_t Size() const { return m_Size; }

public:
    CircularBuffer() = default;

    CircularBuffer(const CircularBuffer& Other) :
        m_Head(Other.m_Head), m_Tail(Other.m_Tail), m_Buffer(Other.m_Buffer), m_Size(Other.m_Size)
    {
    }

    CircularBuffer(CircularBuffer&& Other) noexcept :
        m_Head(std::exchange(Other.m_Head, 0)),
        m_Tail(std::exchange(Other.m_Tail, 0)),
        m_Size(std::exchange(Other.m_Size, 0)),
        m_Buffer(std::move(Other.m_Buffer))
    {
    }

    CircularBuffer& operator=(const CircularBuffer& Other)
    {
        if (this != &Other)
        {
            m_Head   = Other.m_Head;
            m_Tail   = Other.m_Tail;
            m_Size   = Other.m_Size;
            m_Buffer = Other.m_Buffer;
        }
        return *this;
    }

    CircularBuffer& operator=(CircularBuffer&& Other) noexcept
    {
        if (this != &Other)
        {
            m_Head   = std::exchange(Other.m_Head, 0);
            m_Tail   = std::exchange(Other.m_Tail, 0);
            m_Size   = std::exchange(Other.m_Size, 0);
            m_Buffer = std::move(Other.m_Buffer);
        }
        return *this;
    }

public:
    bool operator==(const CircularBuffer& Other) const { return m_Buffer == Other.m_Buffer; }

private:
    std::array<T, BuffSize> m_Buffer;
    // Read
    uint32_t m_Head = 0;
    // Write
    uint32_t m_Tail = 0;
    uint32_t m_Size = 0;
};
} // namespace RA::Bricks
