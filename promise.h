
#ifndef PROMISE_PROMISE_H
#define PROMISE_PROMISE_H

#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>


#ifdef PROMISE_DEBUG_ENABLED
#define DEBUG(...) \
  {                \
    timestamp();   \
    printf(__VA_ARGS__);   \
  }
#else
#define DEBUG(fmt, ...) \
  {}
#endif


enum PromiseState {
  Pending,
  Resolved,
  Rejected
};

#define PromisePtr std::shared_ptr<Promise>
#define PromiseArg const PromisePtr&

inline void timestamp() {
  static auto prev = std::chrono::system_clock::now().time_since_epoch();

  auto time = std::chrono::system_clock::now().time_since_epoch();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time - prev);
  prev = time;

  printf("%lli: ", milliseconds.count());
}

class Promise : public std::enable_shared_from_this<Promise> {
public:

  typedef std::function<void(PromisePtr)> ManuallyResolvedHandler;
  typedef std::function<void()> AutoResolvedHandler;

  ManuallyResolvedHandler manually_resolved_handler = nullptr;
  AutoResolvedHandler auto_resolved_handler = nullptr;

  bool handler_ran = false;
  PromiseState state = Pending;
  // The product of this promise. Passed to child's passed_value promises on Resolve()
  void* data = nullptr;
  // Data from parent Promise.
  void* passed_value = nullptr;

  PromisePtr parent;
  std::vector<PromisePtr> children;

  static void *operator new     (size_t) = delete;
  static void *operator new[]   (size_t) = delete;
  static void  operator delete  (void*)  = delete;
  static void  operator delete[](void*)  = delete;


  uint32_t id = 0;

  explicit Promise() {
    _init();
  };

  explicit Promise(ManuallyResolvedHandler in_handler): manually_resolved_handler(std::move(in_handler)) {
    printf("ManuallyResolvedHandler ");
    _init();
  }
  explicit Promise(AutoResolvedHandler in_handler): auto_resolved_handler(std::move(in_handler)) {
    printf("AutoResolvedHandler ");
    _init();
  }

  void _init() {
    static uint32_t id_counter = 1;
    id = id_counter++;
    DEBUG("Promise [%i] created\n", id);
  }

  ~Promise() {
    DEBUG("Promise [%i] destroyed\n", id);
  }

  template<typename T>
  static PromisePtr New(const T& in_handler) {
    auto promise = std::make_shared<Promise>(in_handler);
    return Register(promise);
  }

  static PromisePtr New() {
    auto promise = std::make_shared<Promise>();
    return Register(promise);
  }

  bool is_registered = false;

  static PromisePtr Register(PromisePtr p) {
    if (p->is_registered)
      return p;
    entries.push_back(p);
    p->is_registered = true;
    return p;
  }

  static void TickAll() {
    for (size_t i = 0; i < entries.size(); i++) { // NOLINT(modernize-loop-convert)
      PromisePtr promise = entries.at(i);
      if (!promise) continue;
      if (promise->IsReadyToExecute()) {
        promise->Tick();
      }

      if (promise && promise->IsDone()) {
        promise->parent.reset();
      }
    }

    const auto size = entries.size();
    const auto it = std::remove_if(entries.begin(), entries.end(),
                                   [](const PromisePtr& promise) {
                                     return !promise || promise->IsDone();
                                   });
    entries.erase(it, entries.end());

    if (size != entries.size()) {
      DEBUG("Cleaned up the vector: %u -> %u\n", size, entries.size());
    }
  }

  static inline std::vector<PromisePtr> entries;

  virtual void Tick() {
    if (!handler_ran) {
      // Setting it before call to have ability to override it in handler
      handler_ran = true;

      if (manually_resolved_handler) {
        manually_resolved_handler(shared_from_this());
      } else if (auto_resolved_handler) {
        auto_resolved_handler();
        Resolve();
      } else {
        Resolve();
      }
    }
  }

  bool IsReadyToExecute() const {
    return (!IsDone()) && (!parent || parent->IsDone());
  }

  bool IsDone() const {
    return state != Pending;
  }

  void Resolve() {
    state = Resolved;
    if (data) {
      for (auto& child : children) {
        child->passed_value = data;
      }
    }
  }

  void ResolveData(void* in_data) {
    data = in_data;
    Resolve();
  }

  void Resolve(const PromisePtr& next) {
    Register(next);

    next->children = children;
    for (auto& child : children) {
      child->parent = next;
    }
    children.clear();
    children.push_back(next);

    Resolve();
  }

  void Resolve(const ManuallyResolvedHandler& next_handler) {
    Resolve(Promise::New(next_handler));
  }

  void Resolve(const AutoResolvedHandler& next_handler) {
    Resolve(Promise::New(next_handler));
  }

  void Reject() {
    state = Rejected;

    for (const auto& child : children) {
      child->Reject(data);
    }
  }

  void Reject(void* in_data) {
    data = in_data;
    Reject();
  }

  template <typename Handler>
  PromisePtr Then(const Handler& next_handler) {
    PromisePtr next = Promise::New<Handler>(next_handler);
    return Then(next);
  }

  PromisePtr Then(const PromisePtr& next) {
    Register(next);
    children.push_back(next);
    next->parent = shared_from_this();
    return next;
  }

};

#undef DEBUG
#undef PROMISE_DEBUG_ENABLED

#endif //PROMISE_PROMISE_H
