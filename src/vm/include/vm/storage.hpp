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
  size_t sweep_count = 0;
  size_t size = 0;
  size_t added_since_last_sweep = 0;

public:
  GCStorage(bool debug = false) : debug{debug} {}
  size_t sweep();

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
