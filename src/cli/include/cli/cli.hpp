#pragma once

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cli {

namespace {
struct string_hash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;

  constexpr std::size_t operator()(const char *str) const {
    return hash_type{}(str);
  }
  constexpr std::size_t operator()(std::string_view str) const {
    return hash_type{}(str);
  }
  constexpr std::size_t operator()(const std::string &str) const {
    return hash_type{}(str);
  }
};

class ParsingContext {
  using raw_argument = const char *const;
  using argument = std::string_view;

public:
  ParsingContext(std::span<const char *const> arguments)
      : arguments(arguments) {}

  [[nodiscard]] argument current() const { return arguments[i]; }
  [[nodiscard]] std::optional<argument> next() const {
    if (i + 1 < arguments.size()) {
      return arguments[i + 1];
    }
    return std::nullopt;
  }

  void advance() { ++i; }
  std::optional<argument> next() {
    if (i < arguments.size()) {
      return arguments[++i];
    }
    return std::nullopt;
  }

  [[nodiscard]] std::size_t size() const { return arguments.size(); }
  [[nodiscard]] bool empty() const { return i >= arguments.size(); }

private:
  std::span<const char *const> arguments;
  std::size_t i = 0;
};

} // namespace

class ParseError : public std::runtime_error {
public:
  ParseError(const std::string &message) : std::runtime_error(message) {}

  template <typename... Args>
  ParseError(const std::format_string<Args...> &message, Args &&...args)
      : std::runtime_error(std::format(message, std::forward<Args>(args)...)) {}

  static ParseError unknown(std::string_view arg);
  static ParseError value(std::string_view arg);
};

class Args {
public:
  [[nodiscard]] bool has_flag(std::string_view name) const;
  [[nodiscard]] std::optional<std::string_view>
  get_value(std::string_view name) const;

  [[nodiscard]] const auto &flags() const { return _flags; }
  [[nodiscard]] const auto &values() const { return _values; }
  [[nodiscard]] std::span<const std::string> positional() const;

private:
  friend class Parser;
  std::unordered_map<std::string, bool, string_hash, std::equal_to<>> _flags;
  std::unordered_map<std::string, std::string, string_hash, std::equal_to<>>
      _values;
  std::vector<std::string> _positional;
};

class Parser {
public:
  Parser &flag(std::string_view short_name, std::string_view long_name);
  Parser &short_flag(std::string_view short_name);
  Parser &long_flag(std::string_view long_name);

  Parser &option(std::string_view short_name, std::string_view long_name);
  Parser &short_option(std::string_view short_name);
  Parser &long_option(std::string_view long_name);

  [[nodiscard]] std::expected<Args, ParseError>
  parse(int argc, char *argv[]) const;

private:
  using NamePair = std::
      pair<std::optional<std::string_view>, std::optional<std::string_view>>;
  std::vector<NamePair> _flags;
  std::vector<NamePair> _options;

  [[nodiscard]] bool
  is_flag(std::string_view short_name, std::string_view long_name) const;

  [[nodiscard]] bool
  is_option(std::string_view short_name, std::string_view long_name) const;

  void set_flag(Args &args, std::string_view name) const;
  void
  set_value(Args &args, std::string_view name, std::string_view value) const;

  [[nodiscard]] static bool find_in(
      const std::vector<NamePair> &vec,
      std::string_view short_name,
      std::string_view long_name
  );

  [[nodiscard]] static std::string_view
  get_store_name(std::string_view short_name, const std::vector<NamePair> &vec);

  std::expected<void, ParseError>
  parse_long_option(ParsingContext &context, Args &args) const;
  std::expected<void, ParseError>
  parse_short_option(ParsingContext &context, Args &args) const;
  std::expected<void, ParseError>
  parse_combined_option(ParsingContext &context, Args &args) const;
};

} // namespace cli
