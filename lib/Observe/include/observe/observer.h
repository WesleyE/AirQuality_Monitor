#pragma once

#include <memory>

namespace observe {

  template <typename... Args> class Event;

  /**
   * A generic event observer class
   */
  class Observer {
  public:
    /**
     * The base class for observer implementations
     */
    struct Base {
      /**
       * Observers should remove themselves from their events once destroyed
       */
      virtual ~Base() {}
    };

    Observer() {}
    Observer(Observer &&other) = default;
    template <typename L> Observer(L &&l) : data(new L(std::move(l))) {}

    Observer &operator=(const Observer &other) = delete;
    Observer &operator=(Observer &&other) = default;

    template <typename L> Observer &operator=(L &&l) {
      data.reset(new L(std::move(l)));
      return *this;
    }

    /**
     * Observe an event with the callback
     */
    template <typename H, typename... Args> void observe(Event<Args...> &event, const H &handler) {
      data.reset(new typename Event<Args...>::Observer(event.createObserver(handler)));
    }

    /**
     * remove the callback from the event
     */
    void reset() { data.reset(); }

    /**
     * returns `true` if currently observing an event
     */
    explicit operator bool() const { return bool(data); }

  private:
    std::unique_ptr<Base> data;
  };

}  // namespace observe