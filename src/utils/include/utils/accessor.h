#pragma once

// Macro to generate getter/setter pairs for a member variable
#define DEFINE_ACCESSOR(name, type, member)                                    \
  [[nodiscard]] const type &get_##name() const { return member; }              \
  [[nodiscard]] type &get_##name##_mut() { return member; }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR(name, type, member)                              \
  [[nodiscard]] type get_##name() const { return member; }                     \
  [[nodiscard]] type &get_##name##_mut() { return member; }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR(name, type, member)                                \
  [[nodiscard]] const type &get_##name() const { return *member; }             \
  [[nodiscard]] type &get_##name##_mut() { return *member; }

// Macro to generate getter/setter pairs for a member variable
#define DEFINE_ACCESSOR_X(name)                                                \
  [[nodiscard]] const decltype(name) &get_##name() const { return name; }      \
  [[nodiscard]] decltype(name) &get_##name##_mut() { return name; }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR_X(name)                                          \
  [[nodiscard]] decltype(name) get_##name() const { return name; }             \
  [[nodiscard]] decltype(name) &get_##name##_mut() { return name; }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR_X(name)                                            \
  [[nodiscard]] const decltype(name)::element_type &get_##name() const {       \
    return *(name);                                                            \
  }                                                                            \
  [[nodiscard]] decltype(name)::element_type &get_##name##_mut() {             \
    return *(name);                                                            \
  }

#define DEREF(name)                                                            \
  [[nodiscard]] auto &operator*() { return (name)(); }                         \
  [[nodiscard]] const auto &operator*() const { return (name)(); }             \
                                                                               \
  auto *operator->() { return &(name)(); }                                     \
  const auto *operator->() const { return &(name)(); }
