#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "StateMachine.h"

using namespace testing;
using ::testing::NiceMock;

namespace ra::bricks::StateMachine
{
// Mock class for IDynamicState<int, std::string>
struct MockState : public IDynamicState<int, std::string>
{
    MOCK_METHOD(void, OnEnter, (), (override));
    MOCK_METHOD(void, OnExit, (), (override));
    MOCK_METHOD((std::pair<std::string, NextState>), Update, (int&&), (override));
};

TEST(DynamicStateMachineTest, NoInitialState)
{
    DynamicStateMachine<int, std::string> machine;
    // Run without setting initial state → should halt and return nullopt
    auto result = machine.Run(1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(machine.GetFSM_State(), StateMachineStatus::Halt);
}

TEST(DynamicStateMachineTest, TransitionAndLifecycleWithMocks)
{
    DynamicStateMachine<int, std::string> machine;

    auto* stateA = new NiceMock<MockState>();
    auto* stateB = new NiceMock<MockState>();

    EXPECT_CALL(*stateA, OnEnter()).Times(1);
    EXPECT_CALL(*stateA, OnExit()).Times(1);

    EXPECT_CALL(*stateA, Update(Eq(5))).WillOnce(Return(std::make_pair("Stay A", stateA)));

    EXPECT_CALL(*stateA, Update(Eq(20))).WillOnce(Return(std::make_pair("To B", stateB)));

    EXPECT_CALL(*stateB, OnEnter()).Times(1);
    EXPECT_CALL(*stateB, OnExit()).Times(1);

    EXPECT_CALL(*stateB, Update(Eq(0))).WillOnce(Return(std::make_pair("Terminate", nullptr)));

    machine.EnterState(stateA);

    auto result = machine.Run(5);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Stay A");

    result = machine.Run(20);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "To B");

    result = machine.Run(0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Terminate");

    EXPECT_EQ(machine.GetFSM_State(), StateMachineStatus::Halt);
}

TEST(DynamicStateMachineTest, TransitionToNullptrTerminatesWithMocks)
{
    DynamicStateMachine<int, std::string> machine;

    auto* stateB = new NiceMock<MockState>();

    EXPECT_CALL(*stateB, OnEnter()).Times(1);
    EXPECT_CALL(*stateB, OnExit()).Times(1);

    EXPECT_CALL(*stateB, Update(Eq(0))).WillOnce(Return(std::make_pair("Terminate", nullptr)));

    machine.EnterState(stateB);

    auto result = machine.Run(0);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Terminate");
    EXPECT_EQ(machine.GetFSM_State(), StateMachineStatus::Halt);
}

TEST(DynamicStateMachineTest, StayInSameStateMultipleRunsWithMocks)
{
    DynamicStateMachine<int, std::string> machine;

    auto* stateA = new NiceMock<MockState>();

    EXPECT_CALL(*stateA, OnEnter()).Times(1);
    EXPECT_CALL(*stateA, OnExit()).Times(0);

    EXPECT_CALL(*stateA, Update(Eq(1))).Times(3).WillRepeatedly(Return(std::make_pair("Stay A", stateA)));

    machine.EnterState(stateA);

    for (int i = 0; i < 3; ++i)
    {
        auto result = machine.Run(1);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), "Stay A");
    }
}

TEST(DynamicStateMachineTest, EnterExitCountsWithMocks)
{
    DynamicStateMachine<int, std::string> machine;

    auto* stateC = new NiceMock<MockState>();
    auto* stateD = new NiceMock<MockState>();

    EXPECT_CALL(*stateC, OnEnter()).Times(1);
    EXPECT_CALL(*stateC, OnExit()).Times(1);

    EXPECT_CALL(*stateD, OnEnter()).Times(1);
    EXPECT_CALL(*stateD, OnExit()).Times(0);

    EXPECT_CALL(*stateC, Update(5)).WillOnce(Return(std::make_pair("Stay C", stateC)));
    EXPECT_CALL(*stateC, Update(20)).WillOnce(Return(std::make_pair("To D", stateD)));

    machine.EnterState(stateC);

    machine.Run(5);  // stays in same state; no enter/exit
    machine.Run(20); // transition to stateD, so OnExit stateC and OnEnter stateD called
}

TEST(DynamicStateMachineTest, SetInitialStateWithNullptr)
{
    DynamicStateMachine<int, std::string> machine;
    auto* state = machine.EnterState(nullptr);
    EXPECT_EQ(state, nullptr);
    EXPECT_EQ(machine.GetFSM_State(), StateMachineStatus::Halt);
}

TEST(DynamicStateMachineTest, RunAfterHaltReturnsNulloptWithMocks)
{
    DynamicStateMachine<int, std::string> machine;

    auto* stateA = new NiceMock<MockState>();

    EXPECT_CALL(*stateA, OnEnter()).Times(1);
    EXPECT_CALL(*stateA, OnExit()).Times(1);

    EXPECT_CALL(*stateA, Update(Eq(20))).WillOnce(Return(std::make_pair("To B", new NiceMock<MockState>())));

    machine.EnterState(stateA);

    machine.Run(20); // transition to B

    auto* stateB = new NiceMock<MockState>();
    EXPECT_CALL(*stateB, OnEnter()).Times(1);
    EXPECT_CALL(*stateB, OnExit()).Times(1);
    EXPECT_CALL(*stateB, Update(Eq(0))).WillOnce(Return(std::make_pair("Terminate", nullptr)));

    machine.EnterState(stateB);
    machine.Run(0); // terminate, set to Halt

    EXPECT_EQ(machine.GetFSM_State(), StateMachineStatus::Halt);

    auto result = machine.Run(5);
    EXPECT_FALSE(result.has_value());
}

} // namespace ra::bricks::StateMachine
