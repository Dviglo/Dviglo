// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../containers/linked_list.h"
#include "string_hash_register.h"
#include "variant.h"

#include <functional>
#include <list>
#include <memory>
#include <utility>


namespace dviglo
{

class Context;
class EventHandler;

#define DV_OBJECT(type_name) \
    public: \
        using ClassName = type_name; \
        static const dviglo::String& GetTypeNameStatic() { static const dviglo::String name(#type_name); return name; } \
        static dviglo::StringHash GetTypeStatic() { static const dviglo::StringHash type(#type_name); return type; } \
        const dviglo::String& GetTypeName() const override { return GetTypeNameStatic(); } \
        dviglo::StringHash GetType() const override { return GetTypeStatic(); }

/// Base class for objects with type identification, subsystem access and event sending/receiving capability.
class DV_API Object : public RefCounted, std::enable_shared_from_this<Object>
{
    friend class Context;

public:
    /// Construct.
    explicit Object();
    /// Destruct. Clean up self from event sender & receiver structures.
    ~Object() override;

    /// Return type hash.
    virtual StringHash GetType() const = 0;

    /// Return type name.
    virtual const String& GetTypeName() const = 0;

    /// Handle event.
    virtual void OnEvent(Object* sender, StringHash eventType, VariantMap& eventData);

    /// Subscribe to an event that can be sent by any sender.
    void subscribe_to_event(StringHash eventType, EventHandler* handler);
    /// Subscribe to a specific sender's event.
    void subscribe_to_event(Object* sender, StringHash eventType, EventHandler* handler);
    /// Subscribe to an event that can be sent by any sender.
    void subscribe_to_event(StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData = nullptr);
    /// Subscribe to a specific sender's event.
    void subscribe_to_event(Object* sender, StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData = nullptr);
    /// Unsubscribe from an event.
    void unsubscribe_from_event(StringHash eventType);
    /// Unsubscribe from a specific sender's event.
    void unsubscribe_from_event(Object* sender, StringHash eventType);
    /// Unsubscribe from a specific sender's events.
    void UnsubscribeFromEvents(Object* sender);
    /// Unsubscribe from all events.
    void UnsubscribeFromAllEvents();
    /// Unsubscribe from all events except those listed, and optionally only those with userdata (script registered events).
    void UnsubscribeFromAllEventsExcept(const Vector<StringHash>& exceptions, bool onlyUserData);
    /// Send event to all subscribers.
    void SendEvent(StringHash eventType);
    /// Send event with parameters to all subscribers.
    void SendEvent(StringHash eventType, VariantMap& eventData);
    /// Return a preallocated map for event data. Used for optimization to avoid constant re-allocation of event data maps.
    VariantMap& GetEventDataMap() const;
    /// Send event with variadic parameter pairs to all subscribers. The parameter pairs is a list of paramID and paramValue separated by comma, one pair after another.
    template <typename... Args> void SendEvent(StringHash eventType, Args... args)
    {
        SendEvent(eventType, GetEventDataMap().Populate(args...));
    }

    /// Return global variable based on key.
    const Variant& GetGlobalVar(StringHash key) const;
    /// Return all global variables.
    const VariantMap& GetGlobalVars() const;
    /// Set global variable with the respective key and value.
    void SetGlobalVar(StringHash key, const Variant& value);
    /// Return active event sender. Null outside event handling.
    Object* GetEventSender() const;
    /// Return active event handler. Null outside event handling.
    EventHandler* GetEventHandler() const;
    /// Return whether has subscribed to an event without specific sender.
    bool HasSubscribedToEvent(StringHash eventType) const;
    /// Return whether has subscribed to a specific sender's event.
    bool HasSubscribedToEvent(Object* sender, StringHash eventType) const;

    /// Return whether has subscribed to any event.
    bool HasEventHandlers() const { return !eventHandlers_.Empty(); }

    /// Return object category. Categories are (optionally) registered along with the object factory. Return an empty string if the object category is not registered.
    const String& GetCategory() const;

    /// Block object from sending and receiving events.
    void SetBlockEvents(bool block) { blockEvents_ = block; }
    /// Return sending and receiving events blocking status.
    bool GetBlockEvents() const { return blockEvents_; }

private:
    /// Find the first event handler with no specific sender.
    EventHandler* FindEventHandler(StringHash eventType, EventHandler** previous = nullptr) const;
    /// Find the first event handler with specific sender.
    EventHandler* FindSpecificEventHandler(Object* sender, EventHandler** previous = nullptr) const;
    /// Find the first event handler with specific sender and event type.
    EventHandler* FindSpecificEventHandler(Object* sender, StringHash eventType, EventHandler** previous = nullptr) const;
    /// Remove event handlers related to a specific sender.
    void RemoveEventSender(Object* sender);

    /// Event handlers. Sender is null for non-specific handlers.
    LinkedList<EventHandler> eventHandlers_;

    /// Block object from sending and receiving any events.
    bool blockEvents_;
};

/// Base class for object factories.
class DV_API ObjectFactory : public RefCounted
{
public:
    /// Construct.
    explicit ObjectFactory()
    {
    }

    /// Create an object. Implemented in templated subclasses.
    virtual SharedPtr<Object> CreateObject() = 0;

    /// Return type hash of objects created by this factory.
    StringHash GetType() const { return type_; }

    /// Return type name of objects created by this factory.
    const String& GetTypeName() const { return type_name_; }

protected:
    StringHash type_;
    String type_name_;
};

/// Template implementation of the object factory.
template <class T> class ObjectFactoryImpl : public ObjectFactory
{
public:
    explicit ObjectFactoryImpl()
    {
        type_ = T::GetTypeStatic();
        type_name_ = T::GetTypeNameStatic();
    }

    /// Create an object of the specific type.
    SharedPtr<Object> CreateObject() override { return SharedPtr<Object>(new T()); }
};

/// Internal helper class for invoking event handler functions.
class DV_API EventHandler : public LinkedListNode
{
public:
    /// Construct with specified receiver and userdata.
    explicit EventHandler(Object* receiver, void* userData = nullptr) :
        receiver_(receiver),
        sender_(nullptr),
        userData_(userData)
    {
    }

    /// Destruct.
    virtual ~EventHandler() = default;

    /// Set sender and event type.
    void SetSenderAndEventType(Object* sender, StringHash eventType)
    {
        sender_ = sender;
        eventType_ = eventType;
    }

    /// Invoke event handler function.
    virtual void Invoke(VariantMap& eventData) = 0;
    /// Return a unique copy of the event handler.
    virtual EventHandler* Clone() const = 0;

    /// Return event receiver.
    Object* GetReceiver() const { return receiver_; }

    /// Return event sender. Null if the handler is non-specific.
    Object* GetSender() const { return sender_; }

    /// Return event type.
    const StringHash& GetEventType() const { return eventType_; }

    /// Return userdata.
    void* GetUserData() const { return userData_; }

protected:
    /// Event receiver.
    Object* receiver_;
    /// Event sender.
    Object* sender_;
    /// Event type.
    StringHash eventType_;
    /// Userdata.
    void* userData_;
};

/// Template implementation of the event handler invoke helper (stores a function pointer of specific class).
template <class T> class EventHandlerImpl : public EventHandler
{
public:
    using HandlerFunctionPtr = void (T::*)(StringHash, VariantMap&);

    /// Construct with receiver and function pointers and userdata.
    EventHandlerImpl(T* receiver, HandlerFunctionPtr function, void* userData = nullptr) :
        EventHandler(receiver, userData),
        function_(function)
    {
        assert(receiver_);
        assert(function_);
    }

    /// Invoke event handler function.
    void Invoke(VariantMap& eventData) override
    {
        auto* receiver = static_cast<T*>(receiver_);
        (receiver->*function_)(eventType_, eventData);
    }

    /// Return a unique copy of the event handler.
    EventHandler* Clone() const override
    {
        return new EventHandlerImpl(static_cast<T*>(receiver_), function_, userData_);
    }

private:
    /// Class-specific pointer to handler function.
    HandlerFunctionPtr function_;
};

/// Template implementation of the event handler invoke helper (std::function instance).
class EventHandler11Impl : public EventHandler
{
public:
    /// Construct with receiver and function pointers and userdata.
    explicit EventHandler11Impl(std::function<void(StringHash, VariantMap&)> function, void* userData = nullptr) :
        EventHandler(nullptr, userData),
        function_(std::move(function))
    {
        assert(function_);
    }

    /// Invoke event handler function.
    void Invoke(VariantMap& eventData) override
    {
        function_(eventType_, eventData);
    }

    /// Return a unique copy of the event handler.
    EventHandler* Clone() const override
    {
        return new EventHandler11Impl(function_, userData_);
    }

private:
    /// Class-specific pointer to handler function.
    std::function<void(StringHash, VariantMap&)> function_;
};

template <typename ... Args>
class Slot;

template <typename ... Args>
class Signal
{
    friend class Slot<Args ...>;

private:
    /// Ссылки на подключённые слоты (не указатели!)
    std::list<std::reference_wrapper<Slot<Args ...>>> slots_;

public:
    ~Signal()
    {
        for (Slot<Args ...>& slot : slots_)
        {
            slot.signal_ = nullptr;
            slot.func_ = nullptr;
        }

        slots_.clear();
    }

    void disconnect(Slot<Args ...>& slot)
    {
        // Если слот был подсоединён к текущему сигналу
        if (slots_.remove_if([&slot](const Slot<Args ...>& val) { return &val == &slot; }))
        {
            // Очищаем слот
            slot.signal_ = nullptr;
            slot.func_ = nullptr;
        }
        else
        {
            // Паникуем
            assert(false);
        }
    }

    void connect(Slot<Args ...>& slot, std::function<void(Args ...)> func)
    {
        assert(func);

        slot.disconnect();

        slot.signal_ = this;
        slot.func_ = func;
        slots_.push_back(slot);
    }

    void emit(Args ... args)
    {
        for (Slot<Args ...>& slot : slots_)
            slot.func_(std::forward<Args>(args) ...);
    }
};

template <typename ... Args>
class Slot
{
    friend class Signal<Args ...>;

private:
    /// Используется для удаления себя из списка слотов в сигнале
    Signal<Args ...>* signal_ = nullptr;

    /// Обработчик события
    std::function<void(Args ...)> func_;

public:
    void connect(Signal<Args ...>& slot, std::function<void(Args ...)> func)
    {
        slot.connect(*this, func);
    }

    void disconnect()
    {
        // Если слот подключён к сигналу
        if (signal_)
        {
            // Удаляем себя из списка слотов в сигнале
            signal_->slots_.remove_if([this](const Slot<Args ...>& val) { return &val == this; });
            signal_ = nullptr;
            func_ = nullptr;
        }
    }

    ~Slot()
    {
        disconnect();
    }
};

/// Get register of event names.
DV_API StringHashRegister& GetEventNameRegister();

/// Describe an event's hash ID and begin a namespace in which to define its parameters.
#define DV_EVENT(eventID, eventName) static const dviglo::StringHash eventID(dviglo::GetEventNameRegister().RegisterString(#eventName)); namespace eventName
/// Describe an event's parameter hash ID. Should be used inside an event namespace.
#define DV_PARAM(paramID, paramName) static const dviglo::StringHash paramID(#paramName)
/// Convenience macro to construct an EventHandler that points to a receiver object and its member function.
#define DV_HANDLER(className, function) (new dviglo::EventHandlerImpl<className>(this, &className::function))
/// Convenience macro to construct an EventHandler that points to a receiver object and its member function, and also defines a userdata pointer.
#define DV_HANDLER_USERDATA(className, function, userData) (new dviglo::EventHandlerImpl<className>(this, &className::function, userData))

} // namespace dviglo
