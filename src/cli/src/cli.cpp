module;

#include <cstdlib>

module l3.cli;

std::optional<std::string_view> ParsingContext::next() {
  if (i + 1 < arguments.size()) {
    return arguments[++i];
  }
  return std::nullopt;
}

namespace cli {

ParseError ParseError::unknown(std::string_view arg) {
  return ParseError{"Unrecognized option '{}'", arg};
}

ParseError ParseError::value(std::string_view arg) {
  return ParseError{"Option '{}' requires a value", arg};
}

std::expected<Args, ParseError>
Parser::parse(int argc, const char *const argv[]) const {
  ParsingContext context{std::span{argv, static_cast<std::size_t>(argc)}};
  auto prog_name = _program_name.empty() ? std::string_view{argv[0]}
                                         : std::string_view{_program_name};

  Args args;
  bool positional_only = false;

  context.advance();

  for (; !context.empty(); context.advance()) {
    const std::string_view arg = context.current();

    if (positional_only || !arg.starts_with("-")) {
      args._positional.emplace_back(arg);
      continue;
    }

    if (arg == "--") {
      positional_only = true;
      continue;
    }

    if (arg == "--help") {
      std::print("{}", generate_help(prog_name));
      std::exit(EXIT_SUCCESS);
    }

    if (arg.starts_with("--")) {
      if (auto result = parse_long_option(context, args); !result) {
        return std::unexpected{result.error()};
      }
      continue;
    }

    if (arg == "-") {
      args._positional.emplace_back(arg);
      continue;
    }

    const auto name = arg.substr(1);

    if (name.size() > 1) {
      if (auto result = parse_combined_option(context, args); !result) {
        return std::unexpected(result.error());
      }
      continue;
    }

    if (name == "h") {
      std::print("{}", generate_help(prog_name));
      std::exit(EXIT_SUCCESS);
    }

    if (auto result = parse_short_option(context, args); !result) {
      return std::unexpected(result.error());
    }
  }

  return args;
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

void Parser::set_flag(Args &args, std::string_view name) const {
  args._flags.emplace(get_store_name(name, _flags), true);
}

void Parser::set_value(
    Args &args, std::string_view name, std::string_view value
) const {
  args._values.emplace(get_store_name(name, _options), value);
}

bool Parser::find_in(
    const std::vector<Entry> &vec,
    std::string_view short_name,
    std::string_view long_name
) {
  return std::ranges::any_of(vec, [&](const auto &entry) {
    return optional_contains(entry.short_name, short_name) ||
           optional_contains(entry.long_name, long_name);
  });
}

std::string_view
Parser::get_store_name(std::string_view name, const std::vector<Entry> &vec) {
  for (const auto &entry : vec) {
    if (optional_contains(entry.short_name, name)) {
      return entry.long_name.value_or(name);
    }
  }
  return name;
}

std::string Parser::generate_help(std::string_view program_name) const {
  std::string result = std::format("Usage: {} [options]", program_name);

  if (!_program_description.empty()) {
    result += "\n\n";
    result += _program_description;
  }

  result += "\n\nOptions:\n";

  struct HelpEntry {
    std::string names;
    std::string_view description;
  };
  std::vector<HelpEntry> entries;

  entries.push_back({"-h, --help", "Show this help message and exit"});

  for (const auto &entry : _flags) {
    std::string names;
    if (entry.short_name && entry.long_name) {
      names = std::format("-{}, --{}", *entry.short_name, *entry.long_name);
    } else if (entry.short_name) {
      names = std::format("-{}", *entry.short_name);
    } else if (entry.long_name) {
      names = std::format("--{}", *entry.long_name);
    }
    if (!names.empty()) {
      entries.push_back({std::move(names), entry.description});
    }
  }

  for (const auto &entry : _options) {
    std::string names;
    if (entry.short_name && entry.long_name) {
      names =
          std::format("-{}, --{} <value>", *entry.short_name, *entry.long_name);
    } else if (entry.short_name) {
      names = std::format("-{} <value>", *entry.short_name);
    } else if (entry.long_name) {
      names = std::format("--{} <value>", *entry.long_name);
    }
    if (!names.empty()) {
      entries.push_back({std::move(names), entry.description});
    }
  }

  std::size_t max_width = 0;
  for (const auto &e : entries) {
    max_width = std::max(max_width, e.names.size());
  }

  constexpr std::size_t padding = 4;
  for (const auto &e : entries) {
    result += "  ";
    result += e.names;
    if (!e.description.empty()) {
      result.append(max_width - e.names.size() + padding, ' ');
      result += e.description;
    }
    result += '\n';
  }

  return result;
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

} // namespace cli
