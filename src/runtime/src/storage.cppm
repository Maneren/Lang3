export module l3.runtime:storage;

import :chunk_list;

export namespace l3::runtime {

class HeapCell;
class Value;

class Heap {
  bool debug;
  ChunkedForwardList<HeapCell, 1024> backing_store;
  std::size_t sweep_count = 0;
  std::size_t size = 0;
  std::size_t added_since_last_sweep = 0;
  std::size_t next_gc_threshold = 1024;

public:
  Heap(bool debug = false);

  Heap(const Heap &) = delete;
  Heap(Heap &&) noexcept;
  Heap &operator=(const Heap &) = delete;
  Heap &operator=(Heap &&) noexcept;
  ~Heap();

  std::size_t sweep();

  HeapCell &emplace(Value &&value);

  DEFINE_VALUE_ACCESSOR_X(debug);
  DEFINE_VALUE_ACCESSOR_X(size);
  DEFINE_VALUE_ACCESSOR_X(sweep_count);
  DEFINE_VALUE_ACCESSOR_X(added_since_last_sweep);
  DEFINE_VALUE_ACCESSOR_X(next_gc_threshold);

private:
  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }
};

} // namespace l3::runtime
