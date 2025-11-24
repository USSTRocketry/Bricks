#pragma once

#include <variant>
#include <optional>
#include <memory>
#include "CircularBuffer.h"

namespace ra::bricks::StateMachine
{
/********************************
 *          Variant
 *******************************/
template <typename... States>
using StateVariant = std::variant<States...>;

template <typename NewState, typename Variant, typename... Arg>
NewState& SwitchVariantState(Variant& States, Arg&&... Args)
{
    std::visit([](auto&& OldState) { OldState.OnExit(); }, States);
    NewState& Ref = States.template emplace<NewState>(std::forward<Args>(Args)...);
    std::visit([](auto&& State) { State.OnEnter(); }, States);
    return Ref;
}

template <typename Input, typename ReturnType, typename StateVariant>
struct IVariantState
{
    virtual void OnEnter() {};
    virtual void OnExit() {};
    /**
     * StateVariant is the storage for the state
     */
    virtual ReturnType Update(Input&&, StateVariant&) = 0;

public:
    virtual ~IVariantState() = default;
};

template <typename Input, typename ReturnType, typename... States>
class VariantStateMachine
{
public:
    /**
     * Performs state action
     */
    ReturnType Run(Input&& In)
    {
        return std::visit(
            [&](auto&& CurrentState)
            {
                // Run the current state's logic, which may modify the state variant to switch state
                return CurrentState.Update(std::forward<Input>(In), m_StatePool);
            },
            m_StatePool);
    }

    template <typename InitState, typename... Args>
    static VariantStateMachine Create(Args&&... args)
    {
        return VariantStateMachine(std::in_place_type<InitState>, std::forward<Args>(args)...);
    }

private:
    template <typename InitState, typename... Args>
    VariantStateMachine(std::in_place_type_t<InitState>, Args&&... args) :
        m_StatePool(std::in_place_type<InitState>, std::forward<Args>(args)...)
    {
    }

    VariantStateMachine(VariantStateMachine&)             = delete;
    VariantStateMachine& operator=(VariantStateMachine&)  = delete;
    VariantStateMachine(VariantStateMachine&&)            = default;
    VariantStateMachine& operator=(VariantStateMachine&&) = default;

    using StateVariant = std::variant<States...>;
    StateVariant m_StatePool;
};

/********************************
 *          Heap allocated
 *******************************/
template <typename Input, typename ReturnType>
struct IDynamicState
{
    using NextState = IDynamicState<Input, ReturnType>*;

    virtual void OnEnter() {};
    virtual void OnExit() {};
    /**
     * @brief Updates the state based on input.
     *
     * @param input Input for the current update cycle.
     * @return Pair of:
     *   - ReturnType: Custom format,
     *   - IDynamicState*:
     *       - `this` → stay in current state,
     *       - different → transition to new state,
     *       - nullptr → terminate.
     *
     * @warning Returned pointer must match the lifetime and ownership model
     *          expected by the state machine (e.g., owning, non-owning, shared).
     */
    virtual auto Update(Input&&) -> std::pair<ReturnType, NextState> = 0;

public:
    virtual ~IDynamicState() = default;
};

template <typename State>
struct IStatePolicy
{
    virtual State* SetState(State* NewState) = 0;
    virtual State* Get() const               = 0;
    virtual void Clear()                     = 0;
};

template <typename State>
struct NonOwningPolicy final : IStatePolicy<State>
{
    using Handle   = State*;
    Handle Storage = nullptr;

    State* SetState(Handle NewState) override
    {
        Storage = NewState;
        return Get();
    }
    State* Get() const override { return Storage; }
    void Clear() override { Storage = nullptr; }
};

template <typename State>
struct OwningPolicy final : IStatePolicy<State>
{
    using Handle = std::unique_ptr<State>;
    Handle Storage;

    // raw ptr adapter
    State* SetState(State* NewState) override { return SetState(Handle {NewState}); }

    State* SetState(Handle&& NewState)
    {
        Storage = std::move(NewState);
        return Get();
    }
    State* Get() const override { return Storage.get(); }
    void Clear() override { Storage.reset(); }
};

// SharedPolicy cannot be used here because we are only working with a raw pointer.
// Instead, use OwningPolicy<SharedState> to manage ownership safely.
// Note: This introduces a performance overhead due to the unique_ptr → shared_ptr → state.run() transition.
enum class StateMachineStatus
{
    Running,
    Halt,
};

/**
 * @brief A dynamic finite state machine with safe deferred transitions.
 *
 * This class manages states implementing `IDynamicState`, allowing each state
 * to request transitions either during its `Update`, `OnEnter`, or `OnExit`
 * callbacks. To prevent infinite recursion and maintain a predictable execution
 * order, transitions requested while the FSM is already running are **deferred**.
 *
 * Key design points:
 * - `Run()` executes a single update cycle and applies at most one deferred transition.
 * - `EnterState()` queues a transition if the FSM is already in progress.
 * - `EnterStateImmediate()` applies a transition synchronously, bypassing the queue.
 * - RAII `InProgressGuard` ensures `m_InProgress` is always correctly set,
 *   preventing nested transitions from causing recursion.
 *
 * Infinite recursion can occur if a state callback synchronously triggered another
 * transition. This class avoids it by deferring transitions until the current
 * update completes, ensuring each state finishes its callbacks before the next
 * transition is processed.
 *
 * Deferred transitions are applied in FIFO order. Urgent transitions can bypass
 * the queue using `EnterStateImmediate()`.
 */
template <typename Input, typename ReturnType, template <typename> class StatePolicy = OwningPolicy>
class DynamicStateMachine
{
    using StateType = IDynamicState<Input, ReturnType>;
    using Policy    = StatePolicy<StateType>;

public:
    /**
     * @brief Executes a single update cycle of the FSM.
     *
     * This processes at most one deferred transition and then updates
     * the current state. If the state requests a transition, it is applied
     * immediately (or queued if the FSM is already in progress).
     *
     * @param In Input for the current update cycle.
     * @return std::optional<ReturnType> The return value from the state's Update().
     *         std::nullopt if there is no active state.
     *
     * @note Only a single unit of work is performed per call. Additional
     *       deferred transitions remain queued for future calls.
     */
    std::optional<ReturnType> Run(Input&& In)
    {
        InProgressGuard Guard {m_InProgress};

        StateType* pS = m_StatePolicy.Get();
        if (pS == nullptr) { return std::nullopt; }

        auto [ReturnVal, pNewState] = pS->Update(std::forward<decltype(In)>(In));

        if (pNewState != pS)
        {
            pS->OnExit();
            StateTransitionDeferred(pNewState);
        }

        {
            const auto NextQueuedState = m_DeferredState.Dequeue();
            if (NextQueuedState.has_value()) { StateTransition(NextQueuedState.value()); }
        }

        return {ReturnVal};
    }
    /**
     * @brief Transition the FSM to the requested state.
     *
     * If the FSM is currently in progress, the state is queued and the
     * active state is returned
     *
     * @param pState Pointer to the state to enter.
     * @return StateType* Pointer to the active state.
     */
    StateType* EnterState(StateType* pState)
    {
        if (m_InProgress) { return StateTransitionDeferred(pState); }

        InProgressGuard Guard {m_InProgress};
        auto Result = StateTransition(pState);

        return Result;
    }
    /**
     * @brief Transition the FSM to the requested state.
     *
     * If the FSM is currently in progress, the state is queued and the
     * active state is returned
     *
     * @param pState Pointer to the state to enter.
     * @return StateType* Pointer to the active state.
     */
    StateType* EnterStateImmediate(StateType* pState)
    {
        InProgressGuard Guard {m_InProgress};
        auto Result = StateTransition(pState);

        return Result;
    }

    StateMachineStatus GetFSM_State() const
    {
        return m_StatePolicy.Get() == nullptr ? StateMachineStatus::Halt : StateMachineStatus::Running;
    }

protected:
    StateType* StateTransitionDeferred(StateType* pState)
    {
        m_DeferredState.Queue(std::move(pState));
        return m_StatePolicy.Get();
    }
    /**
     * @brief Performs a state transition.
     *
     * If the FSM is in progress, the state is queued. Otherwise,
     * the current state is replaced and OnEnter() is called.
     *
     * @param pState Pointer to the new state.
     * @return StateType* Pointer to the active state after the transition,
     */
    StateType* StateTransition(StateType* pState)
    {
        auto pS = m_StatePolicy.SetState(pState);
        if (pS) { pS->OnEnter(); }
        return pS;
    }

private:
    bool m_InProgress {false};
    Policy m_StatePolicy;
    ra::bricks::CircularBuffer<StateType*, 10> m_DeferredState;

private:
    struct InProgressGuard
    {
        bool& Flag;
        explicit InProgressGuard(bool& flag) : Flag(flag) { Flag = true; }
        ~InProgressGuard() { Flag = false; }

        // Disable copying
        InProgressGuard(const InProgressGuard&)            = delete;
        InProgressGuard& operator=(const InProgressGuard&) = delete;
    };
};
} // namespace ra::bricks::StateMachine
