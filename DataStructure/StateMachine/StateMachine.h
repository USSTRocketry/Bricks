#pragma once

#include <variant>
#include <optional>
#include <memory>

namespace RA::Bricks::StateMachine
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

template <typename Input, typename ReturnType, template <typename> class StatePolicy = OwningPolicy>
class DynamicStateMachine
{
    using StateType = IDynamicState<Input, ReturnType>;
    using Policy    = StatePolicy<StateType>;

public:
    std::optional<ReturnType> Run(Input&& In)
    {
        StateType* pS = m_StatePolicy.Get();
        if (pS == nullptr) { return std::nullopt; }

        auto [ReturnVal, pNewState] = pS->Update(std::forward<Input>(In));

        if (pNewState != pS)
        {
            pS->OnExit();
            StateTransition(pNewState);
        }

        return {ReturnVal};
    }

    StateType* EnterState(StateType* pState) { return StateTransition(pState); }

    StateMachineStatus GetState() const
    {
        return m_StatePolicy.Get() == nullptr ? StateMachineStatus::Halt : StateMachineStatus::Running;
    }

protected:
    StateType* StateTransition(StateType* pState)
    {
        auto pS = m_StatePolicy.SetState(pState);
        if (pS) { pS->OnEnter(); }
        return pS;
    }

private:
    Policy m_StatePolicy;
};
} // namespace RA::Bricks::StateMachine
