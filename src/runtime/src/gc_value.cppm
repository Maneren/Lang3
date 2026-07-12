export module l3.runtime:gc_value;

import :value;

export namespace l3::runtime {

class HeapCell {
  HeapData value;
  bool marked = false;

public:
  HeapCell(HeapData &&value);
  HeapCell(const HeapCell &) = delete;
  HeapCell(HeapCell &&other) noexcept;
  HeapCell &operator=(const HeapCell &) = delete;
  HeapCell &operator=(HeapCell &&other) noexcept;
  ~HeapCell() = default;

  void mark();
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::runtime
