export module l3.vm:storage;

import :chunk_list;

export namespace l3::vm {

class GCValue;
class Value;

class GCStorage {
  bool debug;
  ChunkedForwardList<GCValue, 1024> backing_store;
  std::size_t sweep_count = 0;
  std::size_t size = 0;
  std::size_t added_since_last_sweep = 0;

public:
  GCStorage(bool debug = false);

  GCStorage(const GCStorage &) = delete;
  GCStorage(GCStorage &&) noexcept;
  GCStorage &operator=(const GCStorage &) = delete;
  GCStorage &operator=(GCStorage &&) noexcept;
  ~GCStorage();

  std::size_t sweep();

  GCValue &emplace(Value &&value);

  static GCValue &nil() { return NIL; }
  static GCValue &_true() { return TRUE; }
  static GCValue &_false() { return FALSE; }

  DEFINE_VALUE_ACCESSOR_X(debug);
  DEFINE_VALUE_ACCESSOR_X(size);
  DEFINE_VALUE_ACCESSOR_X(sweep_count);
  DEFINE_VALUE_ACCESSOR_X(added_since_last_sweep);

private:
  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  static GCValue NIL;
  static GCValue TRUE;
  static GCValue FALSE;
};

} // namespace l3::vm
