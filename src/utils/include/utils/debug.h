#pragma once

#include <print>
#include <string_view>
#include <type_traits>

namespace utils::debug {

/** @brief Returns the type name of the template parameter T.
 * @tparam T The type to get the name of.
 * @return std::string_view containing the name of the type.
 * @note The output is compiler dependent and not guaranteed to be stable across
 * platforms and versions. Only clang, gcc and msvc are supported.
 */
// Source - https://stackoverflow.com/a/66551751
template <typename T> constexpr auto type_name() -> std::string_view {
#ifdef __clang__
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
  constexpr auto prefix = std::string_view{"[T = "};
  constexpr auto suffix = std::string_view{"]"};
#elifdef __GNUC__
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
  constexpr auto prefix = std::string_view{"with T = "};
  constexpr auto suffix = std::string_view{"; "};
#elifdef _MSC_VER
  constexpr auto function = std::string_view{__FUNCSIG__};
  constexpr auto prefix = std::string_view{"type_name<"};
  constexpr auto suffix = std::string_view{">(void)"};
#else
#error Unsupported compiler
#endif

  constexpr auto prefix_pos = function.find(prefix);
  constexpr auto start_pos = prefix_pos + prefix.size();
  constexpr auto end_pos = function.find(suffix, start_pos);

  static_assert(prefix_pos != std::string_view::npos, "Prefix not found");
  static_assert(end_pos != std::string_view::npos, "Suffix not found");

  return function.substr(start_pos, end_pos - start_pos);
}

/** @brief Returns the type name of the given value.
 * @return std::string_view containing the name of the type.
 * @note The output is compiler dependent and not guaranteed to be stable across
 * platforms and versions. Only clang, gcc and msvc are supported.
 */
auto type_name(const auto &value) -> std::string_view {
  return type_name<std::decay_t<decltype(value)>>();
}

struct ConstructorLogger {
  ConstructorLogger() { std::println("Default constructor"); }
  ConstructorLogger(std::string_view name) : name(name) {
    std::println("Default constructor <{}>", name);
  }
  ConstructorLogger(const ConstructorLogger &other) : name(other.name) {
    std::println("Copy constructor <{}>", name);
  }
  ConstructorLogger(ConstructorLogger &&other) noexcept
      : name(other.name) { // NOLINT
    std::println("Move constructor <{}>", name);
    other.name += " (moved)";
  }
  ConstructorLogger &operator=(const ConstructorLogger &other) {
    std::println("Copy assignment <{}>", name);
    name = other.name;
    return *this;
  }
  ConstructorLogger &operator=(ConstructorLogger &&other) noexcept {
    std::println("Move assignment <{}>", name);
    name = other.name;
    other.name += " (moved)";
    return *this;
  }
  ~ConstructorLogger() { std::println("Destructor <{}>", name); }

private:
  std::string name = "nil";
};

} // namespace utils::debug
