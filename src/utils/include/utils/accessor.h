#pragma once

// Macro to generate a deducing-this getter for a member variable
// Returns const T& / T& depending on the constness of the object
#define DEFINE_ACCESSOR(name, type, member)                                    \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return (self.member);                                                      \
  }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR(name, type, member)                              \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return (self.member);                                                      \
  }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR(name, type, member)                                \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return *(self.member);                                                     \
  }

// Macro to generate a deducing-this getter for a member variable
#define DEFINE_ACCESSOR_X(name)                                                \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return (self.name);                                                        \
  }

// Macro for simple value getters (trivially copy-able members like primitives)
#define DEFINE_VALUE_ACCESSOR_X(name)                                          \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return (self.name);                                                        \
  }

// Macro for pointer/smart pointer accessors with dereference
#define DEFINE_PTR_ACCESSOR_X(name)                                            \
  [[nodiscard]] constexpr decltype(auto) get_##name(this auto &&self) {        \
    return *(self.name);                                                       \
  }

#define DEREF(name)                                                            \
  [[nodiscard]] constexpr decltype(auto) operator*(this auto &&self) {         \
    return (self.name)();                                                      \
  }                                                                            \
                                                                               \
  constexpr auto operator->(this auto &&self) { return &(self.name)(); }

// Macro to add source location field to AST nodes
#define DEFINE_LOCATION_FIELD()                                                \
private:                                                                       \
  location::Location location_;                                                \
                                                                               \
public:                                                                        \
  [[nodiscard]] constexpr const location::Location &get_location() const {     \
    return location_;                                                          \
  }
