[![Actions Status](https://github.com/TheLartians/Observe/workflows/MacOS/badge.svg)](https://github.com/TheLartians/Observe/actions)
[![Actions Status](https://github.com/TheLartians/Observe/workflows/Windows/badge.svg)](https://github.com/TheLartians/Observe/actions)
[![Actions Status](https://github.com/TheLartians/Observe/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/Observe/actions)
[![Actions Status](https://github.com/TheLartians/Observe/workflows/Style/badge.svg)](https://github.com/TheLartians/Observe/actions)
[![Actions Status](https://github.com/TheLartians/Observe/workflows/Install/badge.svg)](https://github.com/TheLartians/Observe/actions)
[![codecov](https://codecov.io/gh/TheLartians/Observe/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/Observe)

# Observe

A thread-safe event-listener template and observable value implementation for C++17.

## API

The core API is best illustrated by an example.

```cpp
#include <string>
#include <iostream>

#include <observe/event.h>

void example() {
  // events can be valueless
  observe::Event<> eventA;

  // or have arguments
  observe::Event<std::string, float> eventB;
  
  // connect will always trigger when an event is triggered
  eventA.connect([](){
    std::cout << "A triggered" << std::endl;
  });
  
  // observers will remove themselves from the event on destroy or reset
  observe::Observer observer = eventB.createObserver([](const std::string &str, float v){ 
    std::cout << "B triggered with " << str << " and " << v << std::endl;
  });

  // call emit to trigger all observers
  eventA.emit();
  eventB.emit("meaning of life", 42);

  // `observe::Observer` can store any type of observer
  // previous observers will be removed
  observer.observe(eventA, [](){ std::cout << "I am now observing A" << std::endl; });

  // to remove an observer without destroying the object, call reset
  observer.reset();
}
```

Note that events and observers are thread and exception safe, as long as the handlers manage their own resources.
Handlers can safely remove observers (including themselves) from the event when beeing called.
Thrown exceptions will propagate out of the `event.emit()` call.

### Using observe::Value

The project also includes a header `observe/value.h` with an experimental observable value implementation.
The API is still subject to change, so use with caution.

```cpp
observe::Value a = 1;
observe::Value b = 2;

// contains the sum of `a` and `b`
observe::DependentObservableValue sum([](auto a, auto b){ return a+b; },a,b);

// all observable values contain an `Event` `onChange`
sum.onChange.connect([](auto &v){ 
  std::cout << "The result changed to " << r << std::endl;
});

// access the value by dereferencing
std::cout << "The result is " << *sum << std::endl; // -> the result is 3

// changes will automatically propagate through dependent values
a.set(3); // -> The result changed to 5
```

## Installation and usage

With [CPM.cmake](https://github.com/TheLartians/CPM) you can easily add the headers to your project.

```cmake
CPMAddPackage(
  NAME Observe
  VERSION 3.0
  GITHUB_REPOSITORY TheLartians/Observe
)

target_link_libraries(myProject Observe)
```
