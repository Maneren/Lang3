module;

#include <print>

module utils;

namespace utils::debug {

ConstructorLogger::ConstructorLogger() { std::println("Default constructor"); }
ConstructorLogger::ConstructorLogger(std::string_view name) : name(name) {
  std::println("Default constructor <{}>", name);
}
ConstructorLogger::ConstructorLogger(const ConstructorLogger &other)
    : name(other.name) {
  std::println("Copy constructor <{}>", name);
}
ConstructorLogger::ConstructorLogger(ConstructorLogger &&other) noexcept
    : name(std::move(other.name)) {
  std::println("Move constructor <{}>", name);
  other.name = name + " (moved)";
}
ConstructorLogger &
ConstructorLogger::operator=(const ConstructorLogger &other) {
  std::println("Copy assignment <{}>", name);
  name = other.name;
  return *this;
}
ConstructorLogger &
ConstructorLogger::operator=(ConstructorLogger &&other) noexcept {
  std::println("Move assignment <{}>", name);
  name = other.name;
  other.name += " (moved)";
  return *this;
}
ConstructorLogger::~ConstructorLogger() {
  std::println("Destructor <{}>", name);
}

} // namespace utils::debug
