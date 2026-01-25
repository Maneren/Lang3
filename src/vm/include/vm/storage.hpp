#pragma once

#include <format>
#include <forward_list>
#include <iostream>
#include <memory>
#include <print>

namespace l3::vm {

class Value;

struct GCValue {
  GCValue(const std::shared_ptr<Value> &value) : value{value} {}
  GCValue(std::shared_ptr<Value> &&value) : value{std::move(value)} {}

  void mark();
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  [[nodiscard]] const Value &get() const { return *value; }
  Value &get() { return *value; }

private:
  bool marked = false;
  std::shared_ptr<Value> value;
};

struct RefValue {
  explicit RefValue(GCValue &gc_value) : gc_value{gc_value} {}

  [[nodiscard]] const Value &get() const { return gc_value.get().get(); }
  [[nodiscard]] Value &get() { return gc_value.get().get(); }

  [[nodiscard]] const GCValue &get_gc() const { return gc_value.get(); }
  [[nodiscard]] GCValue &get_gc() { return gc_value.get(); }

  [[nodiscard]] Value &operator*() { return get(); }
  [[nodiscard]] const Value &operator*() const { return get(); }

  Value *operator->() { return &get(); }
  const Value *operator->() const { return &get(); }

private:
  std::reference_wrapper<GCValue> gc_value;
};

class GCStorage {
public:
  GCStorage(bool debug = false) : debug{debug} {}
  size_t sweep();

  GCValue &emplace(Value &&value);
  GCValue &emplace(const std::shared_ptr<Value> &value);

  static GCValue &nil() { return NIL; }

private:
  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  bool debug;
  std::forward_list<GCValue> backing_store;

  static GCValue NIL;
};

} // namespace l3::vm
