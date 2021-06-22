/*
** Copyright 2020 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef BLOOMBERG_AMQPPROX_EVENTSOURCESIGNAL
#define BLOOMBERG_AMQPPROX_EVENTSOURCESIGNAL

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Bloomberg {
namespace amqpprox {

/**
 *  \brief Base class for desubscription
 *
 * This breaks the circular dependency between the subscription handle and the
 * signal object.
 */
class EventSourceSignalBase
: public std::enable_shared_from_this<EventSourceSignalBase> {
  public:
    virtual ~EventSourceSignalBase() = 0;

    virtual void desubscribe(uint64_t id) = 0;
    ///< Desubscribe the `id` provided, if the `id` is not already
    ///< subscribed this is a no-op.
};

/**
 * \brief RAII handle for storing the subscription lifetime
 *
 * This supports explicitly scoping the lifespan of a particular subscription
 * to the signal. It also supports releasing the subscription early, should the
 * client be interested in doing so.
 *
 * Note that the lifetime of the signal is NOT extended to the lifetime of the
 * handles. They should be desubscribed before the signal is destroyed.
 */
class EventSubscriptionHandle {
    std::weak_ptr<EventSourceSignalBase> d_subscriptionLocation;
    uint64_t                             d_subscriptionId;

  public:
    /**
     * \brief Construct a default `EventSubscriptionHandle`
     */
    EventSubscriptionHandle()
    : d_subscriptionLocation()
    , d_subscriptionId(0)
    {
    }

    /**
     * \brief Construct a `EventSubscriptionHandle` from a location
     * \param location
     * \param id
     */
    EventSubscriptionHandle(
        const std::weak_ptr<EventSourceSignalBase> &location,
        uint64_t                                    id)
    : d_subscriptionLocation(location)
    , d_subscriptionId(id)
    {
    }

    EventSubscriptionHandle(const EventSubscriptionHandle &) = default;
    EventSubscriptionHandle &
    operator=(const EventSubscriptionHandle &) = default;

    EventSubscriptionHandle(EventSubscriptionHandle &&) = default;
    EventSubscriptionHandle &operator=(EventSubscriptionHandle &&) = default;

    /**
     * \brief Explicitly release the subscription
     */
    void release()
    {
        if (!d_subscriptionLocation.expired()) {
            d_subscriptionLocation.lock()->desubscribe(d_subscriptionId);
        }
    }

    ~EventSubscriptionHandle() { release(); }
};

/**
 *  \brief Signal that emits callbacks to all the current subscribers
 */
template <typename... ARGS>
class EventSourceSignal : public EventSourceSignalBase {
  public:
    using SignalCb = std::function<void(ARGS...)>;

  private:
    std::unordered_map<uint64_t, SignalCb> d_subscribers;
    std::mutex                             d_mutex;
    uint64_t                               d_subscribersWatermark;

    EventSourceSignal()
    : d_subscribers()
    , d_mutex()
    , d_subscribersWatermark(0)
    {
    }

  public:
    /**
     * \brief Factory function to create a signal
     * \returns a shared pointer to a newly constructed `EventSourceSignal`
     * instance
     */
    static std::shared_ptr<EventSourceSignal<ARGS...>> create();

    /**
     * \brief Subscribe a callback to receive notifications for this event and
     * return an object that when it goes out of scope will remove the
     * subscription from this signal.
     * \param cb function to be called back when the signal is dispatched
     * \returns a handle to the EventSubscription which can be released later
     * on
     */
    EventSubscriptionHandle subscribe(const SignalCb &cb)
    {
        std::lock_guard<std::mutex> lg(d_mutex);
        uint64_t                    ourId = d_subscribersWatermark++;
        EventSubscriptionHandle     handle(shared_from_this(), ourId);
        d_subscribers[ourId] = cb;
        return handle;
    }

    /**
     * \brief Explicitly remove the subscription for this signal, this is a
     * no-op if the identifier is not a registered subscription.
     * \param id the id of the subscription
     */
    virtual void desubscribe(uint64_t id)
    {
        std::lock_guard<std::mutex> lg(d_mutex);
        d_subscribers.erase(id);
    }

    /**
     * \brief Emit the event to all subscribed handlers
     * \param args parameters to pass through to the callback
     */
    void emit(ARGS... args)
    {
        std::unique_lock<std::mutex>           lock(d_mutex);
        std::unordered_map<uint64_t, SignalCb> subscribers(d_subscribers);

        lock.unlock();  // UNLOCK

        for (auto &&subscriber : subscribers) {
            auto &&cb = subscriber.second;
            cb(args...);
        }
    }
};

template <typename... ARGS>
std::shared_ptr<EventSourceSignal<ARGS...>>
EventSourceSignal<ARGS...>::create()
{
    std::shared_ptr<EventSourceSignal<ARGS...>> signal(
        new EventSourceSignal());
    return std::move(signal);
}

}
}

#endif
