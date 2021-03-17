#include <thread>
#include "promise.h"
#include <set>


void test();


int main() {

  test();

  unsigned int size = 0;

  std::set<uint32_t> prev_ids;

  while (!Promise::entries.empty()) {
    unsigned int new_size = Promise::entries.size();
    if (new_size != size) {
      size = new_size;
      printf("Entries size: %i\n", new_size);
    }
    std::set<uint32_t> ids;
    for (const auto& entry : Promise::entries) {
      ids.insert(entry->id);
    }
    if (prev_ids != ids) {
      printf("Current IDs are: ");
      for (auto id : ids) {
        printf("%i ", id);
      }
      printf("\n");
      prev_ids = ids;
    }

    Promise::TickAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  printf("Entries size: %u\n", Promise::entries.size());

  for (auto& promise: Promise::entries) {
    printf("[%i] %li\n", promise->id, promise.use_count());
  }

  return 0;
}