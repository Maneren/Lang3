export module l3.runtime:upvalue;

import :stack_value;
import :chunk_list;
import std;

export namespace l3::runtime {

class GCUpvalue {
  StackValue value;
  bool marked = false;

public:
  GCUpvalue(StackValue &&value);
  GCUpvalue(const StackValue &value);

  GCUpvalue(const GCUpvalue &) = delete;
  GCUpvalue &operator=(const GCUpvalue &) = delete;
  GCUpvalue(GCUpvalue &&) noexcept = default;
  GCUpvalue &operator=(GCUpvalue &&) noexcept = default;
  ~GCUpvalue() = default;

  void mark();

  void unmark() { marked = false; }
  [[nodiscard]] bool is_marked() const { return marked; }

  [[nodiscard]] StackValue &get() { return value; }
  [[nodiscard]] const StackValue &get() const { return value; }
};

class GCUpvalueStorage {
  ChunkedForwardList<GCUpvalue, 1024> backing_store;

public:
  GCUpvalueStorage() = default;

  GCUpvalueStorage(const GCUpvalueStorage &) = delete;
  GCUpvalueStorage &operator=(const GCUpvalueStorage &) = delete;
  GCUpvalueStorage(GCUpvalueStorage &&) noexcept = default;
  GCUpvalueStorage &operator=(GCUpvalueStorage &&) noexcept = default;
  ~GCUpvalueStorage() = default;

  GCUpvalue &emplace(StackValue &&value) {
    return backing_store.emplace_front(std::move(value));
  }

  GCUpvalue &emplace(const StackValue &value) {
    return backing_store.emplace_front(value);
  }

  std::size_t sweep();
};

} // namespace l3::runtime
