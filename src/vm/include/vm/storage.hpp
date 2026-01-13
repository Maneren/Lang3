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

  void mark() { marked = true; }
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  [[nodiscard]] const Value &get() const { return *value; }
  Value &get() { return *value; }

private:
  bool marked = false;
  std::shared_ptr<Value> value;
};

class GCStorage {
public:
  GCStorage(bool debug = false) : debug{debug} {}
  size_t sweep();

  GCValue &emplace(Value &&value);
  GCValue &emplace(const std::shared_ptr<Value> &value);

private:
  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  bool debug;
  std::forward_list<GCValue> backing_store;
};

} // namespace l3::vm
