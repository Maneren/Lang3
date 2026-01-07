#include "cli/cli.hpp"
#include <algorithm>
#include <functional>

namespace cli {

bool Args::has_flag(std::string_view name) const {
  return _flags.contains(name);
};

std::optional<std::string_view> Args::get_value(std::string_view name) const {
  if (auto it = _values.find(name); it != _values.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::span<const std::string> Args::positional() const { return _positional; }

ParseError ParseError::unknown(std::string_view arg) {
  return ParseError{"Unrecognized option '{}'", arg};
}
ParseError ParseError::value(std::string_view arg) {
  return ParseError{"Option '{}' requires a value", arg};
}

Parser &Parser::flag(std::string_view short_name, std::string_view long_name) {
  _flags.emplace_back(short_name, long_name);
  return *this;
}

Parser &Parser::short_flag(std::string_view short_name) {
  _flags.emplace_back(short_name, std::nullopt);
  return *this;
}

Parser &Parser::long_flag(std::string_view long_name) {
  _flags.emplace_back(std::nullopt, long_name);
  return *this;
}

Parser &
Parser::option(std::string_view short_name, std::string_view long_name) {
  _options.emplace_back(short_name, long_name);
  return *this;
}

Parser &Parser::short_option(std::string_view short_name) {
  _options.emplace_back(short_name, std::nullopt);
  return *this;
}

Parser &Parser::long_option(std::string_view long_name) {
  _options.emplace_back(std::nullopt, long_name);
  return *this;
}
bool Parser::is_flag(
    std::string_view short_name, std::string_view long_name
) const {
  return find_in(_flags, short_name, long_name);
}
bool Parser::is_option(
    std::string_view short_name, std::string_view long_name
) const {
  return find_in(_options, short_name, long_name);
}

namespace {

template <typename T>
  requires requires(T name) { name == name; }
bool optional_contains(std::optional<T> opt, T name) {
  return opt.transform(std::bind_front(std::equal_to<>(), name))
      .value_or(false);
}

} // namespace

bool Parser::find_in(
    const std::vector<NamePair> &vec,
    std::string_view short_name,
    std::string_view long_name
) {
  return std::ranges::any_of(vec, [&](const auto pair) {
    const auto &[s, l] = pair;
    return optional_contains(s, short_name) || optional_contains(l, long_name);
  });
}
[[nodiscard]] std::string_view Parser::get_store_name(
    std::string_view name, const std::vector<NamePair> &vec
) {
  for (const auto &[s, l] : vec) {
    if (optional_contains(s, name)) {
      return l.value_or(name);
    }
  }
  return name;
}

std::expected<void, ParseError>
Parser::parse_long_option(ParsingContext &context, Args &args) const {
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

std::expected<void, ParseError>
Parser::parse_short_option(ParsingContext &context, Args &args) const {
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

std::expected<void, ParseError>
Parser::parse_combined_option(ParsingContext &context, Args &args) const {
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

std::expected<Args, ParseError> Parser::parse(int argc, char *argv[]) const {
  auto context = ParsingContext{std::span{argv, static_cast<size_t>(argc)}};

  Args args;
  bool positional_only = false;

  context.advance(); // skip executable name

  for (; !context.empty(); context.advance()) {
    std::string_view arg = context.current();

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

void Parser::set_flag(Args &args, std::string_view name) const {
  args._flags.emplace(get_store_name(name, _flags), true);
}
void Parser::set_value(
    Args &args, std::string_view name, std::string_view value
) const {
  args._values.emplace(get_store_name(name, _options), value);
}

} // namespace cli
