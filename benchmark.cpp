#include "benchmark/benchmark.h"

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash.h"
#include "absl/random/random.h"
#include "absl/synchronization/mutex.h"

namespace {

template <class K, class V>
class ContendedHashTable {
 public:
  template <class Cont>
  void Reset(const Cont& c) {
    map_ = {c.begin(), c.end()};
  }

  void Insert(const K& k, const V& v) {
    absl::MutexLock lock(&mu_);
    map_.emplace(k, v);
  }

  V GetFromMap(const K& k, const V& v) {
    absl::MutexLock lock(&mu_);
    auto it = map_.find(k);
    if (ABSL_PREDICT_FALSE(it == map_.end())) {
      it = map_.emplace(k, v).first;
    }
    return it->second;
  }

  V GetFromMapWithHint(const K& k, const V& v) {
    const size_t hint_hash = absl::Hash<K>()(k);
    absl::MutexLock lock(&mu_);
    auto it = map_.find(k, hint_hash);
    if (ABSL_PREDICT_FALSE(it == map_.end())) {
      it = map_.emplace(k, v).first;
    }
    return it->second;
  }

 private:
  absl::Mutex mu_;
  absl::flat_hash_map<K, V> map_ ABSL_GUARDED_BY(mu_);
};

std::string UniformString(absl::BitGen& bits, size_t len) {
  std::string bytes(len, '0');
  for (size_t i = 0; i < len; ++i) {
    bytes[i] = absl::Uniform<uint8_t>(bits, 0, 255);
  }
  return bytes;
}

std::vector<std::pair<std::string, uint64_t>> storage;
ContendedHashTable<std::string_view, uint64_t> table;

template <bool kUseHinted>
void BM_HashTable(benchmark::State& state) {
  static constexpr int kMaxMemory = 200'000'000;  // 200MB
  const size_t string_size = state.range(0);
  const size_t hashtable_size = state.range(1);
  if (string_size * hashtable_size > kMaxMemory) {
    state.SkipWithError("Test is too big");
    return;
  }
  absl::BitGen bits;
  if (state.thread_index() == 0) {
    // Setup code here.
    storage.resize(hashtable_size);
    for (auto& [str, value] : storage) {
      str = UniformString(bits, string_size);
      value = absl::Uniform<uint64_t>(bits, 0,
                                      std::numeric_limits<uint64_t>::max());
    }
    table.Reset(storage);
  }
  static constexpr int kRandomIndicesBufferSize = 100000;
  std::vector<size_t> random_indices(kRandomIndicesBufferSize);
  for (size_t& index : random_indices) {
    index = absl::Uniform<size_t>(bits, 0, storage.size() - 1);
  }
  size_t i = 0;
  for (auto _ : state) {
    if constexpr (!kUseHinted) {
      benchmark::DoNotOptimize(table.GetFromMap(
          storage[random_indices[i]].first, storage[random_indices[i]].second));
    } else {
      benchmark::DoNotOptimize(table.GetFromMapWithHint(
          storage[random_indices[i]].first, storage[random_indices[i]].second));
    }
    ++i;
    if (i == kRandomIndicesBufferSize) {
      state.PauseTiming();
      for (size_t& index : random_indices) {
        index = absl::Uniform<size_t>(bits, 0, storage.size() - 1);
      }
      i = 0;
      state.ResumeTiming();
    }
  }
  state.counters["String Size"] = string_size;
  state.counters["HashTable Size"] = hashtable_size;
  state.counters["Use hinted"] = kUseHinted;
}

BENCHMARK_TEMPLATE(BM_HashTable, false)
    ->RangePair(1, 4096, 10, 10000000)
    ->ThreadRange(1, 16);
BENCHMARK_TEMPLATE(BM_HashTable, true)
    ->RangePair(1, 4096, 10, 10000000)
    ->ThreadRange(1, 16);

}  // namespace

BENCHMARK_MAIN();
