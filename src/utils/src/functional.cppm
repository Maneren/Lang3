module;

#include <utility>

export module utils:functional;

export namespace functional {

template <typename T> struct Constructor {
  template <typename... Args> T operator()(Args &&...args) const {
    return T(std::forward<Args>(args)...);
  }
};

inline void drop(...) {}

} // namespace functional
