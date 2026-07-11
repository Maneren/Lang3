export module l3.runtime:upvalue;

import :stack_value;
import :chunk_list;
import std;

export namespace l3::runtime {

class UpvalueCell {
  StackValue value;
  bool marked = false;

public:
  UpvalueCell(StackValue &&value);
  UpvalueCell(const StackValue &value);

  UpvalueCell(const UpvalueCell &) = delete;
  UpvalueCell &operator=(const UpvalueCell &) = delete;
  UpvalueCell(UpvalueCell &&) noexcept = default;
  UpvalueCell &operator=(UpvalueCell &&) noexcept = default;
  ~UpvalueCell() = default;

  void mark();

  void unmark() { marked = false; }
  [[nodiscard]] bool is_marked() const { return marked; }

  [[nodiscard]] StackValue &get() { return value; }
  [[nodiscard]] const StackValue &get() const { return value; }
};

class UpvalueStorage {
  ChunkedForwardList<UpvalueCell, 1024> backing_store;

public:
  UpvalueStorage() = default;

  UpvalueStorage(const UpvalueStorage &) = delete;
  UpvalueStorage &operator=(const UpvalueStorage &) = delete;
  UpvalueStorage(UpvalueStorage &&) noexcept = default;
  UpvalueStorage &operator=(UpvalueStorage &&) noexcept = default;
  ~UpvalueStorage() = default;

  UpvalueCell &emplace(StackValue &&value) {
    return backing_store.emplace_front(std::move(value));
  }

  UpvalueCell &emplace(const StackValue &value) {
    return backing_store.emplace_front(value);
  }

  std::size_t sweep();
};

} // namespace l3::runtime
