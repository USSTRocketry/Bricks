#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <span>
#include <cstddef>

#include "Random.h"
#include "CachedBuffer.h"

using namespace ra::bricks;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
auto& RNG = ra::RNG::Instance();

class CachedBufferTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Buffer = std::make_unique<CachedBuffer<8>>(
            [](std::span<const std::byte> Data, void* Args) -> uint32_t
            {
                auto* mock = reinterpret_cast<MockStoreCallback*>(Args);
                return mock->Call(Data, Args);
            },
            &m_MockCallback);
    }

    std::unique_ptr<CachedBuffer<8>> m_Buffer;

protected:
    struct MockStoreCallback
    {
        MOCK_METHOD(uint32_t, Call, (std::span<const std::byte> Data, void* Args), ());
    };

    NiceMock<MockStoreCallback> m_MockCallback;
};

// Test storing data smaller than buffer
TEST_F(CachedBufferTest, StoreSmallData_CachesSuccessfully)
{
    const auto Data = RNG.ByteVector(3);

    // Callback should not be called for small data
    EXPECT_CALL(m_MockCallback, Call(_, _)).Times(0);

    bool Result = m_Buffer->Store(Data);
    EXPECT_TRUE(Result);
}

// Test buffer flush when data exceeds remaining space
TEST_F(CachedBufferTest, StoreData_TriggersFlushWhenFull)
{
    const auto FirstData  = RNG.ByteVector(5);
    const auto SecondData = RNG.ByteVector(4);
    std::vector<std::byte> Out;

    // Expect flush for first batch when second batch cannot fit
    EXPECT_CALL(m_MockCallback, Call(_, _))
        .WillOnce(
            [&](std::span<const std::byte> Data, void*)
            {
                Out.insert(Out.end(), Data.begin(), Data.end());
                return static_cast<uint32_t>(Data.size_bytes());
            });

    m_Buffer->Store(FirstData);
    bool Result = m_Buffer->Store(SecondData);
    EXPECT_TRUE(Result);
    EXPECT_TRUE(FirstData == Out);
}

// Test storing data larger than buffer
TEST_F(CachedBufferTest, StoreDataLargerThanBuffer_WritesDirectly)
{
    // Larger than buffer of size 8
    const auto LargeData = RNG.ByteVector(16);

    EXPECT_CALL(m_MockCallback, Call(_, _))
        .WillOnce(
            [&](std::span<const std::byte> Data, void*)
            {
                EXPECT_EQ(Data.size_bytes(), LargeData.size());
                EXPECT_TRUE(memcmp(Data.data(), LargeData.data(), Data.size_bytes()) == 0);
                return static_cast<uint32_t>(Data.size_bytes());
            });

    bool Result = m_Buffer->Store(LargeData);
    EXPECT_TRUE(Result);
}

// Test flush
TEST_F(CachedBufferTest, FlushWritesPendingData)
{
    const auto Data = RNG.ByteVector(2);

    EXPECT_CALL(m_MockCallback, Call(_, _)).WillOnce(Return(Data.size()));

    m_Buffer->Store(Data);
    m_Buffer->Flush(); // Should trigger callback
}

// Test store failure does not corrupt buffer
TEST_F(CachedBufferTest, StoreFailure_DoesNotModifyBuffer)
{
    const auto Data = RNG.ByteVector(8);

    EXPECT_CALL(m_MockCallback, Call(_, _)).WillOnce(Return(0)); // Simulate failure

    bool Result = m_Buffer->Store(Data);
    m_Buffer->Flush();
    EXPECT_TRUE(Result);
    EXPECT_TRUE(m_Buffer->Size() == Data.size());
}

// Test move constructor
TEST_F(CachedBufferTest, MoveConstructor_MovesBufferCorrectly)
{
    const auto Data = RNG.ByteVector(2);

    EXPECT_CALL(m_MockCallback, Call(_, _)).Times(0);

    m_Buffer->Store(Data);

    CachedBuffer<8> MovedBuffer(std::move(*m_Buffer));
    // Moved buffer should still store new data
    const auto MoreData = RNG.ByteVector(1);
    bool Result         = MovedBuffer.Store(MoreData);
    EXPECT_TRUE(Result);
}

// Test copy constructor
TEST_F(CachedBufferTest, CopyConstructor_CopiesBufferCorrectly)
{
    const auto Data = RNG.ByteVector(2);
    EXPECT_CALL(m_MockCallback, Call(_, _)).Times(0);
    m_Buffer->Store(Data);

    CachedBuffer<8> CopiedBuffer(*m_Buffer);
    const auto MoreData = RNG.ByteVector(1);
    bool Result         = CopiedBuffer.Store(MoreData);
    EXPECT_TRUE(Result);
}
