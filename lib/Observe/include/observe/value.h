#pragma once

#include <observe/event.h>

#include <tuple>
#include <type_traits>
#include <utility>

namespace observe {

  namespace value_detail {
    // source: https://stackoverflow.com/questions/6534041/how-to-check-whether-operator-exists
    struct No {};
    template <typename T, typename Arg> No operator==(const T &, const Arg &);
    template <typename T, typename Arg = T> struct HasEqual {
      enum { value = !std::is_same<decltype(*(T *)(0) == *(Arg *)(0)), No>::value };
    };
  }  // namespace value_detail

  template <class T> class Value {
  protected:
    T value;

  public:
    using OnChange = Event<const T &>;
    OnChange onChange;

    template <typename... Args> Value(Args... args) : value(std::forward<Args>(args)...) {}

    Value(Value &&) = delete;
    Value &operator=(Value &&) = delete;

    template <typename... Args> void set(Args &&... args) {
      if constexpr (value_detail::HasEqual<T>::value) {
        T newValue(std::forward<Args>(args)...);
        if (value != newValue) {
          value = std::move(newValue);
          onChange.emit(value);
        }
      } else {
        value = T(std::forward<Args>(args)...);
        onChange.emit(value);
      }
    }

    template <typename... Args> void setSilently(Args... args) {
      value = T(std::forward<Args>(args)...);
    }

    explicit operator const T &() const { return value; }

    const T &get() const { return value; }

    const T &operator*() const { return value; }

    const T *operator->() const { return &value; }
  };

  template <class T> Value(T)->Value<T>;

  template <class T, typename... D> class DependentObservableValue : public Value<T> {
  private:
    std::tuple<typename Value<D>::OnChange::Observer...> observers;

  public:
    template <class H> DependentObservableValue(const H &handler, const Value<D> &... deps)
        : Value<T>(handler(deps.get()...)),
          observers(std::make_tuple(deps.onChange.createObserver(
              [&, this](const auto &) { this->set(handler(deps.get()...)); })...)) {}
  };

  template <class F, typename... D> DependentObservableValue(F, const Value<D> &...)
      ->DependentObservableValue<typename std::invoke_result<F, const D &...>::type, D...>;

}  // namespace observe
