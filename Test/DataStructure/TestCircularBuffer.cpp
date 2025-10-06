#include "gtest/gtest.h"
#include "CircularBuffer.h"

namespace RA::Bricks
{
// Test that the buffer starts empty
TEST(CircularBufferTest, InitialState)
{
    CircularBuffer<int> buffer;

    EXPECT_FALSE(buffer.Dequeue().has_value());
}

// Test enqueueing and dequeueing one element
TEST(CircularBufferTest, EnqueueDequeueSingleElement)
{
    CircularBuffer<int> buffer;

    // Enqueue an element
    EXPECT_TRUE(buffer.Queue(10));

    // Dequeue the element
    auto item = buffer.Dequeue();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 10);
}

// Test that trying to dequeue from an empty buffer returns std::nullopt
TEST(CircularBufferTest, DequeueEmptyBuffer)
{
    CircularBuffer<int> buffer;

    auto item = buffer.Dequeue();
    EXPECT_FALSE(item.has_value()); // The buffer should be empty
}

// Test when the buffer is full
TEST(CircularBufferTest, FullBuffer)
{
    CircularBuffer<int, 2> buffer;

    // Enqueue two elements
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));

    // Try to enqueue a third element, which should fail
    EXPECT_FALSE(buffer.Queue(30));
}

// Test wrapping of the circular buffer (overwrite behavior)
TEST(CircularBufferTest, WrapAround)
{
    CircularBuffer<int, 3> buffer;

    // Enqueue three elements
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));
    EXPECT_TRUE(buffer.Queue(30));

    // Try to enqueue a fourth element, this should not overwrite and be rejected
    EXPECT_FALSE(buffer.Queue(40));

    // Dequeue the items and check the order
    auto item1 = buffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 10);

    auto item2 = buffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 20);

    EXPECT_TRUE(buffer.Queue(80));
    EXPECT_TRUE(buffer.Queue(90));
    EXPECT_FALSE(buffer.Queue(100));

    // should get [30, 80, 90]
    auto item3 = buffer.Dequeue();
    ASSERT_TRUE(item3.has_value());
    EXPECT_EQ(item3.value(), 30);

    item1 = buffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 80);

    item2 = buffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 90);
}

// Test copy constructor
TEST(CircularBufferTest, CopyConstructor)
{
    CircularBuffer<int, 3> buffer;

    // Enqueue some data
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));

    // Copy the buffer
    CircularBuffer<int, 3> copiedBuffer = buffer;
    EXPECT_EQ(copiedBuffer.Size(), buffer.Size());

    // Dequeue from the copied buffer and verify the state
    auto item1 = copiedBuffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 10);

    auto item2 = copiedBuffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 20);
}

// Test move constructor
TEST(CircularBufferTest, MoveConstructor)
{
    CircularBuffer<int, 3> buffer;

    // Enqueue some data
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));

    // Move the buffer
    CircularBuffer<int, 3> movedBuffer = std::move(buffer);

    // The original buffer should be empty now
    EXPECT_FALSE(buffer.Dequeue().has_value());
    EXPECT_EQ(buffer.Size(), 0);
    EXPECT_EQ(movedBuffer.Size(), 2);

    // Dequeue from the moved buffer
    auto item1 = movedBuffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 10);

    auto item2 = movedBuffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 20);
}

// Test copy assignment operator
TEST(CircularBufferTest, CopyAssignment)
{
    CircularBuffer<int, 3> buffer;
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));

    CircularBuffer<int, 3> copiedBuffer;
    copiedBuffer = buffer;
    EXPECT_EQ(copiedBuffer.Size(), buffer.Size());

    // Dequeue from the copied buffer and verify the state
    auto item1 = copiedBuffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 10);

    auto item2 = copiedBuffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 20);
}

// Test move assignment operator
TEST(CircularBufferTest, MoveAssignment)
{
    CircularBuffer<int, 3> buffer;
    EXPECT_TRUE(buffer.Queue(10));
    EXPECT_TRUE(buffer.Queue(20));

    CircularBuffer<int, 3> movedBuffer;
    movedBuffer = std::move(buffer);
    EXPECT_EQ(movedBuffer.Size(), 2);

    // The original buffer should be empty now
    EXPECT_EQ(buffer.Size(), 0);

    // Dequeue from the moved buffer
    auto item1 = movedBuffer.Dequeue();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(item1.value(), 10);

    auto item2 = movedBuffer.Dequeue();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(item2.value(), 20);
}
} // namespace RA::Bricks
