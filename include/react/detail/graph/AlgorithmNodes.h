
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>

#include "EventNodes.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FoldBaseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class FoldBaseNode : public SignalNode<D,S>
{
public:
    template <typename T>
    FoldBaseNode(T&& init, const SharedPtrT<EventStreamNode<D,E>>& events) :
        SignalNode(std::forward<T>(init)),
        events_{ events }
    {}

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        S newValue = calcNewValue();

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! impl::Equals(newValue, value_))
        {
            value_ = newValue;
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual int DependencyCount() const    override    { return 1; }

protected:
    SharedPtrT<EventStreamNode<D,E>> events_;

    virtual S calcNewValue() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class FoldNode : public FoldBaseNode<D,S,E>
{
public:
    template <typename T, typename F>
    FoldNode(T&& init, const SharedPtrT<EventStreamNode<D,E>>& events, F&& func) :
        FoldBaseNode(std::forward<T>(init), events),
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~FoldNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override    { return "FoldNode"; }

protected:
    TFunc   func_;

    virtual S calcNewValue() const override
    {
        S newValue = value_;
        for (const auto& e : events_->Events())
            newValue = func_(newValue,e);
            
        return newValue;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class IterateNode : public FoldBaseNode<D,S,E>
{
public:
    template <typename T, typename F>
    IterateNode(T&& init, const SharedPtrT<EventStreamNode<D,E>>& events, F&& func) :
        FoldBaseNode(std::forward<T>(init), events),
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~IterateNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override    { return "IterateNode"; }

protected:
    TFunc   func_;

    virtual S calcNewValue() const override
    {
        S newValue = value_;
        for (const auto& e : events_->Events())
            newValue = func_(newValue);
        return newValue;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class HoldNode : public SignalNode<D,S>
{
public:
    template <typename T>
    HoldNode(T&& init, const SharedPtrT<EventStreamNode<D,S>>& events) :
        SignalNode(std::forward<T>(init)),
        events_{ events }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events_);
    }

    ~HoldNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override    { return "HoldNode"; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool changed = false;

        if (! events_->Events().empty())
        {
            const S& newValue = events_->Events().back();
            if (newValue != value_)
            {
                changed = true;
                value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual int DependencyCount() const override    { return 1; }

private:
    const SharedPtrT<EventStreamNode<D,S>>    events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class SnapshotNode : public SignalNode<D,S>
{
public:
    SnapshotNode(const SharedPtrT<SignalNode<D,S>>& target,
                 const SharedPtrT<EventStreamNode<D,E>>& trigger) :
        SignalNode{ target->ValueRef() },
        target_{ target },
        trigger_{ trigger }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~SnapshotNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SnapshotNode"; }
    virtual int         DependencyCount() const override    { return 2; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash()));

        bool changed = false;
        
        if (! trigger_->Events().empty())
        {
            const S& newValue = target_->ValueRef();
            if (newValue != value_)
            {
                changed = true;
                value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash()));

        if (changed)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

private:
    const SharedPtrT<SignalNode<D,S>>       target_;
    const SharedPtrT<EventStreamNode<D,E>>  trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class MonitorNode : public EventStreamNode<D,E>
{
public:
    MonitorNode(const SharedPtrT<SignalNode<D,E>>& target) :
        EventStreamNode{ },
        target_{ target }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
    }

    ~MonitorNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "MonitorNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

private:
    const SharedPtrT<SignalNode<D,E>>    target_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class PulseNode : public EventStreamNode<D,S>
{
public:
    PulseNode(const SharedPtrT<SignalNode<D,S>>& target,
              const SharedPtrT<EventStreamNode<D,E>>& trigger) :
        EventStreamNode{ },
        target_{ target },
        trigger_{ trigger }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~PulseNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "PulseNode"; }
    virtual int         DependencyCount() const override    { return 2; }

    virtual void Tick(void* turnPtr) override
    {
        typedef typename D::Engine::TurnT TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);
        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        for (const auto& e : trigger_->Events())
            events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

private:
    const SharedPtrT<SignalNode<D,S>>       target_;
    const SharedPtrT<EventStreamNode<D,E>>  trigger_;
};

/****************************************/ REACT_IMPL_END /***************************************/