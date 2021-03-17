#include "promise.h"
#include "delay.h"

PromisePtr createLoopedDelay() {
  static int max = 5;
  if (!--max) {
    printf("DONE LOOP\n");
    auto p = Promise::New();
    p->Resolve();
    return p;
  }

  timestamp(); printf("Creating Looped Delay\n");
  return Promise::Register(std::make_shared<Delay>(2000))
      ->Then([](const std::shared_ptr<Promise>& p) {
        timestamp(); printf("Delay Resolved LOOPED\n");
        p->Resolve(createLoopedDelay());
      });
}


void test() {

  Promise::New(([](PromiseArg promise){

    timestamp(); printf("Hello!\n");
    promise->Resolve();

  }))->Then([](PromiseArg promise) {
    timestamp(); printf("I'll reject\n");

    promise->Resolve([](PromiseArg promise){
      timestamp(); printf("Before bye\n");
      promise->Resolve();
    });

    promise->Reject();

  })->Then([](PromiseArg promise) {

    timestamp(); printf("Bye! I'll hang on here for a while\n");

    Promise::Register(std::make_shared<Delay>([promise](PromiseArg p) {
      timestamp(); printf("Done.\n");
      p->Resolve();
      promise->Resolve();
    }, 3000));

  })->Then([](PromiseArg p) {
    timestamp(); printf("Returning Delay\n");
    const std::shared_ptr<Delay>& ptr = std::make_shared<Delay>(7000);
    p->Resolve(ptr);
  })->Then([]() -> void {
    timestamp(); printf("Should resolve after\n");
  });
}
