module;

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module cli;

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
  constexpr ParsingContext(std::span<const char *const> arguments)
      : arguments(arguments) {}

  [[nodiscard]] constexpr argument current() const { return arguments[i]; }
  constexpr void advance() { ++i; }
  std::optional<argument> next() {
    if (i + 1 < arguments.size()) {
      return arguments[++i];
    }
    return std::nullopt;
  }

  [[nodiscard]] constexpr std::size_t size() const { return arguments.size(); }
  [[nodiscard]] constexpr bool empty() const { return i >= arguments.size(); }

private:
  std::span<const char *const> arguments;
  std::size_t i = 0;
};

namespace {

template <typename T>
  requires requires(T name) { name == name; }
constexpr bool optional_contains(std::optional<T> opt, T name) {
  return opt.transform(std::bind_front(std::equal_to<>(), name))
      .value_or(false);
}

} // namespace

} // namespace

export namespace cli {

class ParseError : public std::runtime_error {
public:
  constexpr explicit ParseError(const std::string &message)
      : std::runtime_error(message) {}

  template <typename... Args>
  constexpr explicit ParseError(
      const std::format_string<Args...> &message, Args &&...args
  )
      : std::runtime_error(std::format(message, std::forward<Args>(args)...)) {}

  constexpr static ParseError unknown(std::string_view arg) {
    return ParseError{"Unrecognized option '{}'", arg};
  }
  constexpr static ParseError value(std::string_view arg) {
    return ParseError{"Option '{}' requires a value", arg};
  }
};

class Args {
public:
  [[nodiscard]] constexpr bool has_flag(std::string_view name) const {
    return _flags.contains(name);
  }
  [[nodiscard]] constexpr std::optional<std::string_view>
  get_value(std::string_view name) const {
    if (auto it = _values.find(name); it != _values.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] constexpr const auto &flags() const { return _flags; }
  [[nodiscard]] constexpr const auto &values() const { return _values; }
  [[nodiscard]] constexpr std::span<const std::string> positional() const {
    return _positional;
  }

private:
  friend class Parser;
  std::unordered_map<std::string, bool, string_hash, std::equal_to<>> _flags;
  std::unordered_map<std::string, std::string, string_hash, std::equal_to<>>
      _values;
  std::vector<std::string> _positional;
};

class Parser {
public:
  constexpr Parser &
  flag(std::string_view short_name, std::string_view long_name) {
    _flags.emplace_back(short_name, long_name);
    return *this;
  }

  constexpr Parser &short_flag(std::string_view short_name) {
    _flags.emplace_back(short_name, std::nullopt);
    return *this;
  }

  constexpr Parser &long_flag(std::string_view long_name) {
    _flags.emplace_back(std::nullopt, long_name);
    return *this;
  }

  constexpr Parser &
  option(std::string_view short_name, std::string_view long_name) {
    _options.emplace_back(short_name, long_name);
    return *this;
  }

  constexpr Parser &short_option(std::string_view short_name) {
    _options.emplace_back(short_name, std::nullopt);
    return *this;
  }

  constexpr Parser &long_option(std::string_view long_name) {
    _options.emplace_back(std::nullopt, long_name);
    return *this;
  }

  [[nodiscard]] constexpr std::expected<Args, ParseError> parse(
      int argc, const char *const argv[] // NOLINT(modernize-avoid-c-arrays)

  ) const {
    ParsingContext context{std::span{argv, static_cast<std::size_t>(argc)}};

    Args args;
    bool positional_only = false;

    context.advance(); // skip executable name

    for (; !context.empty(); context.advance()) {
      const std::string_view arg = context.current();

      if (positional_only || !arg.starts_with("-")) {
        args._positional.emplace_back(arg);
        continue;
      }

      // After "--" all arguments are considered positional
      if (arg == "--") {
        positional_only = true;
        continue;
      }

      // Long option: --name or --name=value
      if (arg.starts_with("--")) {
        if (auto result = parse_long_option(context, args); !result) {
          return std::unexpected{result.error()};
        }
        continue;
      }

      // Check for '-' special case
      if (arg == "-") {
        // '-' is a positional argument
        args._positional.emplace_back(arg);
        continue;
      }

      const auto name = arg.substr(1);

      // Combined option: -abc
      if (name.size() > 1) {
        if (auto result = parse_combined_option(context, args); !result) {
          return std::unexpected(result.error());
        }
        continue;
      }

      // Simple short option: -n or -n value
      if (auto result = parse_short_option(context, args); !result) {
        return std::unexpected(result.error());
      }
    }

    return args;
  }

private:
  using NamePair = std::
      pair<std::optional<std::string_view>, std::optional<std::string_view>>;
  std::vector<NamePair> _flags;
  std::vector<NamePair> _options;

  [[nodiscard]] constexpr bool
  is_flag(std::string_view short_name, std::string_view long_name) const {
    return find_in(_flags, short_name, long_name);
  }

  [[nodiscard]] constexpr bool
  is_option(std::string_view short_name, std::string_view long_name) const {
    return find_in(_options, short_name, long_name);
  }

  constexpr void set_flag(Args &args, std::string_view name) const {
    args._flags.emplace(get_store_name(name, _flags), true);
  }
  constexpr void
  set_value(Args &args, std::string_view name, std::string_view value) const {
    args._values.emplace(get_store_name(name, _options), value);
  }

  [[nodiscard]] constexpr static bool find_in(
      const std::vector<NamePair> &vec,
      std::string_view short_name,
      std::string_view long_name
  ) {
    return std::ranges::any_of(vec, [&](const auto &pair) {
      const auto &[s, l] = pair;
      return optional_contains(s, short_name) ||
             optional_contains(l, long_name);
    });
  }

  [[nodiscard]] constexpr static std::string_view
  get_store_name(std::string_view name, const std::vector<NamePair> &vec) {
    for (const auto &[s, l] : vec) {
      if (optional_contains(s, name)) {
        return l.value_or(name);
      }
    }
    return name;
  }

  constexpr std::expected<void, ParseError>
  parse_long_option(ParsingContext &context, Args &args) const {
    auto name = context.current().substr(2);
    std::optional<std::string_view> inline_value;

    if (auto eq_pos = name.find('='); eq_pos != std::string_view::npos) {
      inline_value = name.substr(eq_pos + 1);
      name = name.substr(0, eq_pos);
    }

    if (is_flag("", name)) {
      if (inline_value) {
        return std::unexpected(
            ParseError{"Flag '--{}' does not take a value", name}
        );
      }
      set_flag(args, name);
      return {};
    }

    if (is_option("", name)) {
      if (auto value = inline_value.or_else([&] { return context.next(); });
          value) {
        set_value(args, name, *value);
        return {};
      }

      return std::unexpected(ParseError::value(context.current()));
    }

    return std::unexpected(ParseError::unknown(context.current()));
  }
  constexpr std::expected<void, ParseError>
  parse_short_option(ParsingContext &context, Args &args) const {
    auto name = context.current().substr(1);
    auto short_name = name.substr(0, 1);

    if (is_flag(short_name, "")) {
      set_flag(args, short_name);
    } else if (is_option(short_name, "")) {
      if (auto value = context.next(); value) {
        set_value(args, short_name, *value);
      } else {
        return std::unexpected(ParseError::value(context.current()));
      }
    } else {
      return std::unexpected(ParseError::unknown(context.current()));
    }

    return {};
  }
  constexpr std::expected<void, ParseError>
  parse_combined_option(ParsingContext &context, Args &args) const {
    auto arg = context.current().substr(1);

    for (char c : arg) {
      auto name = std::string_view(&c, 1);
      if (!is_flag(name, "")) {
        return std::unexpected(ParseError{"Unrecognized option '-{}'", c});
      }
      set_flag(args, name);
    }

    return {};
  }
};

} // namespace cli
