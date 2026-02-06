export module l3.vm:stack;

import :ref_value;

export namespace l3::vm {

class Stack {
  bool debug;
  std::vector<std::vector<RefValue>> frames;

  class FrameGuard {
    Stack &stack;
    size_t frame_index;

  public:
    explicit FrameGuard(Stack &stack);
    FrameGuard(const FrameGuard &) = delete;
    FrameGuard(FrameGuard &&) = delete;
    FrameGuard &operator=(const FrameGuard &) = delete;
    FrameGuard &operator=(FrameGuard &&) = delete;
    ~FrameGuard();
  };

public:
  Stack(bool debug = false);

  FrameGuard with_frame();
  RefValue push_value(RefValue value);

  void mark_gc();

  DEFINE_ACCESSOR_X(frames);
  DEFINE_VALUE_ACCESSOR_X(debug);

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }
};

} // namespace l3::vm
