#pragma once

#include <observe/observer.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace observe {

  template <typename... Args> class SharedEvent;

  template <typename... Args> class Event {
  public:
    /**
     * The handler type for this event
     */
    using Handler = std::function<void(const Args &...)>;

  private:
    using HandlerID = size_t;

    /**
     * Stores an event handler
     */
    struct StoredHandler {
      HandlerID id;
      std::shared_ptr<Handler> callback;
    };

    using HandlerList = std::vector<StoredHandler>;

    struct Data {
      HandlerID IDCounter = 0;
      HandlerList observers;
      std::mutex observerMutex;
    };

    /**
     * Contains the event's data and handlers
     * Observers should store a `weak_ptr` to the data to observe event lifetime
     */
    std::shared_ptr<Data> data;

    HandlerID addHandler(Handler h) const {
      std::lock_guard<std::mutex> lock(data->observerMutex);
      data->observers.emplace_back(StoredHandler{data->IDCounter, std::make_shared<Handler>(h)});
      return data->IDCounter++;
    }

  protected:
    /**
     * Copy and assignment is `protected` to prevent accidental duplication of the event and its
     * handlers. If you need this, use `SharedEvent` instead.
     */
    Event(const Event &) = default;
    Event &operator=(const Event &) = default;

  public:
    /**
     * The specific Observer implementation for this event
     */
    class Observer : public observe::Observer::Base {
    private:
      std::weak_ptr<Data> data;
      HandlerID id;

    public:
      Observer() {}
      Observer(const std::weak_ptr<Data> &_data, HandlerID _id) : data(_data), id(_id) {}

      Observer(Observer &&other) = default;
      Observer(const Observer &other) = delete;

      Observer &operator=(const Observer &other) = delete;
      Observer &operator=(Observer &&other) = default;

      /**
       * Observe another event of the same type
       */
      void observe(const Event &event, const Handler &handler) {
        reset();
        *this = event.createObserver(handler);
      }

      /**
       * Removes the handler from the event
       */
      void reset() {
        if (auto d = data.lock()) {
          std::lock_guard<std::mutex> lock(d->observerMutex);
          auto it = std::find_if(d->observers.begin(), d->observers.end(),
                                 [&](auto &o) { return o.id == id; });
          if (it != d->observers.end()) {
            d->observers.erase(it);
          }
        }
        data.reset();
      }

      ~Observer() { reset(); }
    };

    Event() : data(std::make_shared<Data>()) {}

    Event(Event &&other) : Event() { *this = std::move(other); }

    Event &operator=(Event &&other) {
      std::swap(data, other.data);
      return *this;
    }

    /**
     * Call all handlers currently connected to the event in the order they were added (thread
     * safe). If a handler is removed before its turn (by another thread or previous handler) it
     * will not be called.
     */
    void emit(Args... args) const {
      std::vector<std::weak_ptr<Handler>> handlers;
      {
        std::lock_guard<std::mutex> lock(data->observerMutex);
        handlers.resize(data->observers.size());
        std::transform(data->observers.begin(), data->observers.end(), handlers.begin(),
                       [](auto &h) { return h.callback; });
      }
      for (auto &weakCallback : handlers) {
        if (auto callback = weakCallback.lock()) {
          (*callback)(args...);
        }
      }
    }

    /**
     * Add a temporary handler to the event.
     * The handlers lifetime will be managed by the returned observer object.
     */
    Observer createObserver(const Handler &h) const { return Observer(data, addHandler(h)); }

    /**
     * Add a permanent handler to the event.
     */
    HandlerID connect(const Handler &h) const { return addHandler(h); }

    /**
     * Remove a permanent handler from the event.
     */
    void disconnect(HandlerID id) const { Observer(data, id).reset(); }

    /**
     * Remove all handlers (temporary and permanent) connected to the event.
     */
    void reset() const {
      std::lock_guard<std::mutex> lock(data->observerMutex);
      data->observers.clear();
    }

    /**
     * The number of observers connected to the event.
     */
    size_t observerCount() const {
      std::lock_guard<std::mutex> lock(data->observerMutex);
      return data->observers.size();
    }
  };

  /**
   * An event class that can be copied and assigned.
   * Behaves just like a more efficient `std::shared_ptr<Event<Args...>>` without derefencing.
   */
  template <typename... Args> class SharedEvent : public Event<Args...> {
  public:
    using Event<Args...>::Event;

    SharedEvent(const SharedEvent<Args...> &other) : Event<Args...>(other) {}

    SharedEvent &operator=(const SharedEvent<Args...> &other) {
      Event<Args...>::operator=(other);
      return *this;
    }
  };

}  // namespace observe
