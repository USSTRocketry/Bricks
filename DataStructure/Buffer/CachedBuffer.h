#pragma once

#include <cstdint>
#include <cstddef>

#include <array>
#include <span>
#include <cassert>

namespace ra::bricks
{
template <uint32_t BufferSize = 512>
class CachedBuffer
{
public:
    /*
     * Call back function when cache reaches its prescribed limit
     * @return bytes written
     *
     * std::function might heap allocate
     */
    using StoreCallback = uint32_t (*)(std::span<const std::byte> Data, void* Args);

public:
    /*
     * Cache messages to internal buffer, Calls StoreCallback once cache is full
     * Will not store partial Data :
     * if Data fits in cache
     *   save to cache
     * else if Data does not fit in cache
     *   flush the cache
     *   flush the new data
     *
     * On store failure, no modification to the cache buffer will occur, that means
     * you will not loose data
     *
     * @return successfully stored
     */
    bool Store(std::span<const std::byte> Data)
    {
        // if message larger than max size, write directly
        if (Data.size_bytes() > m_Buffer.size())
        {
            if (!EmptyBufferCache()) { return false; }

            return m_Callback(Data, m_CbArgs) == Data.size_bytes();
        }

        const auto BufferSpace = m_Buffer.size() - m_BufferOffset;
        if (Data.size_bytes() > BufferSpace)
        {
            if (!EmptyBufferCache()) { return false; }
        }

        std::memcpy(m_Buffer.data() + m_BufferOffset, Data.data(), Data.size_bytes());
        m_BufferOffset += Data.size_bytes();
        return true;
    }
    void Flush() { EmptyBufferCache(); }

    size_t Size() const { return m_BufferOffset; }

    void RegisterCallback(StoreCallback Callback, void* Args)
    {
        m_Callback = Callback;
        m_CbArgs   = Args;
    }

public:
    explicit CachedBuffer(StoreCallback Callback, void* Args) : m_Callback(Callback), m_CbArgs(Args) {}
    CachedBuffer() = default;

    CachedBuffer(const CachedBuffer& Other) :
        m_Buffer(Other.m_Buffer),
        m_BufferOffset(Other.m_BufferOffset),
        m_Callback(Other.m_Callback),
        m_CbArgs(Other.m_CbArgs)
    {
    }

    CachedBuffer& operator=(const CachedBuffer& Other)
    {
        if (this != &Other)
        {
            m_Buffer       = Other.m_Buffer;
            m_BufferOffset = Other.m_BufferOffset;
            m_Callback     = Other.m_Callback;
            m_CbArgs       = Other.m_CbArgs;
        }
        return *this;
    }

    CachedBuffer(CachedBuffer&& Other) noexcept :
        m_Buffer(std::move(Other.m_Buffer)),
        m_BufferOffset(std::exchange(Other.m_BufferOffset, 0)),
        m_Callback(std::exchange(Other.m_Callback, nullptr)),
        m_CbArgs(std::exchange(Other.m_CbArgs, nullptr))
    {
    }

    CachedBuffer& operator=(CachedBuffer&& Other) noexcept
    {
        if (this != &Other)
        {
            m_Buffer       = std::move(Other.m_Buffer);
            m_BufferOffset = std::exchange(Other.m_BufferOffset, 0);
            m_Callback     = std::exchange(Other.m_Callback, nullptr);
            m_CbArgs       = std::exchange(Other.m_CbArgs, nullptr);
        }
        return *this;
    }

private:
    bool EmptyBufferCache()
    {
        if (m_BufferOffset == 0) { return true; }

        const auto BytesWritten = m_Callback({m_Buffer.data(), m_BufferOffset}, m_CbArgs);

        if (BytesWritten == 0) { return false; }
        else {
            // Shift the buffer forward
            const auto BytesLeft = m_BufferOffset - BytesWritten;
            std::memcpy(m_Buffer.data(), m_Buffer.data() + BytesWritten, BytesLeft);

            m_BufferOffset = BytesLeft;
        }
        return true;
    }

private:
    std::array<std::byte, BufferSize> m_Buffer {};
    size_t m_BufferOffset    = 0;
    // default prevents crashing
    StoreCallback m_Callback = [](auto, auto*)
    {
        assert(true);
        return 0;
    };
    void* m_CbArgs {nullptr};
};
} // namespace ra::bricks
