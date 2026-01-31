#pragma once

// Macro to generate getter/setter pairs for a member variable
#define DEFINE_ACCESSOR(name, type, member)                                    \
  [[nodiscard]] constexpr const type &get_##name() const { return member; }    \
  [[nodiscard]] constexpr type &get_##name##_mut() { return member; }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR(name, type, member)                              \
  [[nodiscard]] constexpr type get_##name() const { return member; }           \
  [[nodiscard]] constexpr type &get_##name##_mut() { return member; }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR(name, type, member)                                \
  [[nodiscard]] constexpr const type &get_##name() const { return *member; }   \
  [[nodiscard]] constexpr type &get_##name##_mut() { return *member; }

// Macro to generate getter/setter pairs for a member variable
#define DEFINE_ACCESSOR_X(name)                                                \
  [[nodiscard]] constexpr const decltype(name) &get_##name() const {           \
    return name;                                                               \
  }                                                                            \
  [[nodiscard]] constexpr decltype(name) &get_##name##_mut() { return name; }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR_X(name)                                          \
  [[nodiscard]] constexpr decltype(name) get_##name() const { return name; }   \
  [[nodiscard]] constexpr decltype(name) &get_##name##_mut() { return name; }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR_X(name)                                            \
  [[nodiscard]] constexpr const decltype(name)::element_type &get_##name()     \
      const {                                                                  \
    return *(name);                                                            \
  }                                                                            \
  [[nodiscard]] constexpr decltype(name)::element_type &get_##name##_mut() {   \
    return *(name);                                                            \
  }

#define DEREF(name)                                                            \
  [[nodiscard]] constexpr auto &operator*() { return (name)(); }               \
  [[nodiscard]] constexpr const auto &operator*() const { return (name)(); }   \
                                                                               \
  constexpr auto *operator->() { return &(name)(); }                           \
  constexpr const auto *operator->() const { return &(name)(); }
