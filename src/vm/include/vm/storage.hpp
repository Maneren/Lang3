#pragma once

#include "utils/accessor.h"
#include "vm/value.hpp"
#include <forward_list>
#include <iostream>
#include <memory>
#include <print>

namespace l3::vm {

class GCValue {
  bool marked = false;
  Value value;

public:
  GCValue(Value &&value);
  GCValue(const GCValue &) = delete;
  GCValue(GCValue &&other) noexcept
      : marked{other.marked}, value{std::move(other.value)} {
    other.marked = false;
  }
  GCValue &operator=(const GCValue &) = delete;
  GCValue &operator=(GCValue &&other) noexcept {
    marked = other.marked;
    value = std::move(other.value);
    other.marked = false;
    return *this;
  }
  ~GCValue() = default;

  void mark();
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  DEFINE_ACCESSOR_X(value);
};

class GCStorage {
  bool debug;
  std::forward_list<GCValue> backing_store;

public:
  GCStorage(bool debug = false) : debug{debug} {}
  size_t sweep();

  GCValue &emplace(Value &&value);
  GCValue &emplace(std::unique_ptr<Value> &&value);

  static GCValue &nil() { return NIL; }

  DEFINE_VALUE_ACCESSOR_X(debug);

private:
  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  static GCValue NIL;
};

} // namespace l3::vm
