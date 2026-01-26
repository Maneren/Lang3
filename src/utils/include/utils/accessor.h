#pragma once

// Macro to generate getter/setter pairs for a member variable
#define DEFINE_ACCESSOR(name, type, member)                                    \
  [[nodiscard]] const type &get_##name() const { return member; }              \
  type &get_##name##_mut() { return member; }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR(name, type, member)                              \
  [[nodiscard]] type get_##name() const { return member; }                     \
  type &get_##name##_mut() { return member; }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR(name, type, member)                                \
  [[nodiscard]] const type &get_##name() const { return *member; }             \
  type &get_##name##_mut() { return *member; }
