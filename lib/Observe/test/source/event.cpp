#include <doctest/doctest.h>
#include <observe/event.h>

using namespace observe;

// instantiate template for coverage
template class observe::Event<>;

TEST_CASE("connect and observe") {
  observe::Event<> event;
  CHECK(event.observerCount() == 0);
  unsigned connectCount = 0, observeCount = 0;
  event.connect([&]() { connectCount++; });

  SUBCASE("reset observer") {
    observe::Event<>::Observer observer;
    observer = event.createObserver([&]() { observeCount++; });
    for (int i = 0; i < 10; ++i) {
      event.emit();
    }
    CHECK(event.observerCount() == 2);
    observer.reset();
    CHECK(event.observerCount() == 1);
    for (int i = 0; i < 10; ++i) {
      event.emit();
    }
    CHECK(observeCount == 10);
    CHECK(connectCount == 20);
  }

  SUBCASE("scoped observer") {
    SUBCASE("observe::Observer") {
      observe::Observer observer;
      observer.observe(event, [&]() { observeCount++; });
      CHECK(event.observerCount() == 2);
      for (int i = 0; i < 10; ++i) {
        event.emit();
      }
    }
    SUBCASE("observe::Event<>::Observer") {
      observe::Event<>::Observer observer;
      observer.observe(event, [&]() { observeCount++; });
      CHECK(event.observerCount() == 2);
      for (int i = 0; i < 10; ++i) {
        event.emit();
      }
    }
    CHECK(event.observerCount() == 1);
    for (int i = 0; i < 10; ++i) {
      event.emit();
    }
    CHECK(observeCount == 10);
    CHECK(connectCount == 20);
  }

  SUBCASE("clear observers") {
    observe::Observer observer = event.createObserver([&]() { observeCount++; });
    event.reset();
    CHECK(event.observerCount() == 0);
    event.emit();
    CHECK(connectCount == 0);
  }
}

TEST_CASE("removing observer during emit") {
  observe::Event<> event;
  observe::Event<>::Observer observer;
  unsigned count = 0;
  SUBCASE("self removing") {
    observer = event.createObserver([&]() {
      observer.reset();
      count++;
    });
    event.emit();
    CHECK(count == 1);
    event.emit();
    CHECK(count == 1);
  }
  SUBCASE("other removing") {
    event.connect([&]() { observer.reset(); });
    observer = event.createObserver([&]() { count++; });
    event.emit();
    CHECK(count == 0);
    event.emit();
    CHECK(count == 0);
  }
}

TEST_CASE("adding observers during emit") {
  observe::Event<> event;
  std::function<void()> callback;
  callback = [&]() { event.connect(callback); };
  event.connect(callback);
  CHECK(event.observerCount() == 1);
  event.emit();
  CHECK(event.observerCount() == 2);
  event.emit();
  CHECK(event.observerCount() == 4);
}

TEST_CASE("emit data") {
  observe::Event<int, int> event;
  int sum = 0;
  event.connect([&](auto a, auto b) { sum = a + b; });
  event.emit(2, 3);
  CHECK(sum == 5);
}

TEST_CASE("move") {
  observe::Observer observer;
  int result = 0;
  observe::Event<int> event;
  {
    observe::Event<int> tmpEvent;
    observer = tmpEvent.createObserver([&](auto i) { result = i; });
    tmpEvent.emit(5);
    CHECK(result == 5);
    event = Event(std::move(tmpEvent));
    CHECK(tmpEvent.observerCount() == 0);
  }
  CHECK(event.observerCount() == 1);
  event.emit(3);
  CHECK(result == 3);
  observer.reset();
  CHECK(event.observerCount() == 0);
}

TEST_CASE("SharedEvent") {
  observe::SharedEvent<> onA, onB;
  observe::SharedEvent<> onR(onA);
  unsigned aCount = 0, bCount = 0;
  onR.connect([&]() { aCount++; });
  onA.emit();
  onR = onB;
  onR.connect([&]() { bCount++; });
  onB.emit();
  CHECK(aCount == 1);
  CHECK(bCount == 1);
}

TEST_CASE("connect and disconnect") {
  observe::Event event;

  unsigned counter = 0;
  auto id = event.connect([&]() { counter++; });
  event.emit();
  CHECK(counter == 1);

  SUBCASE("loose id") {
    id = decltype(id)();
    event.emit();
    CHECK(counter == 2);
  }

  SUBCASE("disconnect") {
    event.disconnect(id);
    event.emit();
    CHECK(counter == 1);
  }
}