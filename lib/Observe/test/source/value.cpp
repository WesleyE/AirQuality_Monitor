#include <doctest/doctest.h>
#include <observe/value.h>

using namespace observe;

// instantiate templates for coverage
template class observe::Value<int>;
template class observe::DependentObservableValue<int, int, int>;

TEST_CASE("Value") {
  int current = 0;
  Value value(current);
  unsigned changes = 0;
  value.onChange.connect([&](auto &v) {
    REQUIRE(current == v);
    changes++;
  });
  REQUIRE(*value == 0);
  REQUIRE(static_cast<const int &>(value) == 0);
  current++;
  value.set(current);
  value.set(current);
  value.set(current);
  current++;
  value.set(current);
  value.set(current);
  REQUIRE(changes == 2);
  REQUIRE(*value == 2);
}

TEST_CASE("Value without comparison operator") {
  struct A {};
  Value<A> value;
  unsigned changes = 0;
  value.onChange.connect([&](auto &) { changes++; });
  value.set();
  value.set();
  value.set();
  REQUIRE(changes == 3);
}

TEST_CASE("Dependent Observable Value") {
  Value a(1);
  Value b(1);
  DependentObservableValue sum([](auto a, auto b) { return a + b; }, a, b);

  REQUIRE(*sum == 2);
  a.set(2);
  REQUIRE(*sum == 3);
  b.set(3);
  REQUIRE(*sum == 5);

  Value c(3);
  DependentObservableValue prod([](auto a, auto b) { return a * b; }, sum, c);

  REQUIRE(*prod == 15);
  a.set(1);
  REQUIRE(*prod == 12);
  b.set(4);
  REQUIRE(*prod == 15);
  c.set(2);
  REQUIRE(*prod == 10);

  c.setSilently(3);
  REQUIRE(*prod == 10);
}

TEST_CASE("Operators") {
  using namespace observe;
  struct A {
    int a = 0;
  };
  Value<A> value;
  REQUIRE(value->a == 0);
}