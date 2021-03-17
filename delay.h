#ifndef PROMISE_DELAY_H
#define PROMISE_DELAY_H

#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include "promise.h"

typedef std::chrono::time_point<std::chrono::system_clock> time_point;

class Delay : public Promise {
public:

  unsigned long ms;
  time_point target_time;

  explicit Delay(ManuallyResolvedHandler _handler, unsigned long _ms): Delay(_ms) {
    manually_resolved_handler = std::move(_handler);
  }

  explicit Delay(unsigned long _ms): ms(_ms), Promise() {
    const time_point& time = std::chrono::system_clock::now();
    target_time = time + std::chrono::milliseconds(ms);
    std::chrono::duration<double> duration = target_time - time;
    timestamp(); printf("Time diff: %f\n", duration.count());
  }

  void Tick() override {
    time_point time = std::chrono::system_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time - target_time);
    if (milliseconds.count() >= 0) {
      Promise::Tick();
    }
  }
};


#endif //PROMISE_DELAY_H
