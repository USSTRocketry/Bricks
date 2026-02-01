#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace ra::bricks
{
/**
 * @brief Fixed-size circular (ring) buffer supporting move-only semantics.
 *
 * This class implements a simple circular buffer (FIFO queue) with a
 * statically defined capacity. It is designed for use with move-constructible
 * types and does not perform any dynamic memory allocation.
 *
 * @tparam T         Type of elements stored in the buffer. Must be move-constructible.
 * @tparam BuffSize  Maximum number of elements the buffer can hold.
 */
template <typename T, uint32_t BuffSize = 64>
class CircularBuffer
{
    static_assert(std::is_move_constructible_v<T>, "T must be move-constructible");

public:
    /**
     * @brief Adds an item to the buffer if space is available.
     *
     * @param Item Item to enqueue.
     * @return true if added, false if buffer is full.
     */
    bool Queue(auto&& Item)
    {
        if (m_Size == BuffSize) { return false; }

        m_Buffer[m_Tail] = std::forward<decltype(Item)>(Item);
        m_Tail           = Advance(m_Tail);
        ++m_Size;

        return true;
    }
    /**
     * @brief Get the next allocated obj in the buffer if available.
     *
     * @return containing the next allocated T, or std::nullopt if empty.
     */
    std::optional<std::reference_wrapper<T&>> Allocate()
    {
        if (m_Size == BuffSize) { return std::nullopt; }

        auto& Item = m_Buffer[m_Tail];
        m_Tail     = Advance(m_Tail);
        ++m_Size;

        return {Item};
    }
    /**
     * @brief Dequeues an item from the buffer.
     *
     * Removes and returns the oldest item currently stored in the buffer.
     * If the buffer is empty, returns std::nullopt.
     *
     * @return std::optional<T> containing the dequeued item, or std::nullopt if empty.
     */
    std::optional<T> Dequeue()
    {
        if (m_Size == 0) { return std::nullopt; }

        auto& item = m_Buffer[m_Head];
        m_Head     = Advance(m_Head);
        --m_Size;

        return std::move(item);
    }
    /**
     * @brief Provides a reference to the next item in the buffer without removing it.
     *
     * Returns a reference to the element at the head of the buffer wrapped in
     * a std::optional. If the buffer is empty, std::nullopt is returned.
     *
     * @return Reference to the next available item, or std::nullopt if empty.
     *
     * @note The returned reference remains valid only as long as the underlying
     *       buffer is not modified (e.g., until the next successful call to
     *       Dequeue() or Queue()). Accessing it after modification results in
     *       undefined behavior.
     */
    std::optional<std::reference_wrapper<T>> Peek()
    {
        if (m_Size == 0) { return std::nullopt; }

        return {m_Buffer[m_Head]};
    }

    /**
     * @brief Gets the number of elements currently stored in the buffer.
     *
     * @return The current element count.
     */
    uint32_t Empty() const { return m_Size == 0; }
    /**
     * @brief Gets the number of elements currently stored in the buffer.
     *
     * @return The current element count.
     */
    uint32_t Size() const { return m_Size; }
    /**
     * @brief Gets the number of elements currently stored in the buffer.
     *
     * @return The current element count.
     */
    uint32_t MaxCapacity() const { return BuffSize; }

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

protected:
    uint32_t Advance(uint32_t Pos, uint32_t Amount = 1) { return (Pos + Amount) % BuffSize; }

private:
    std::array<T, BuffSize> m_Buffer;
    // Read
    uint32_t m_Head = 0;
    // Write
    uint32_t m_Tail = 0;
    uint32_t m_Size = 0;
};
} // namespace ra::bricks
