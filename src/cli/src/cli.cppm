export module l3.cli;

import std;

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
  constexpr ParsingContext(std::span<raw_argument> arguments)
      : arguments(arguments) {}

  [[nodiscard]] constexpr argument current() const { return arguments[i]; }
  constexpr void advance() { ++i; }
  std::optional<argument> next();

  [[nodiscard]] constexpr std::size_t size() const { return arguments.size(); }
  [[nodiscard]] constexpr bool empty() const { return i >= arguments.size(); }

private:
  std::span<raw_argument> arguments;
  std::size_t i = 0;
};

template <typename T>
  requires requires(T name) { name == name; }
constexpr bool optional_contains(std::optional<T> opt, T name) {
  return opt.transform(std::bind_front(std::equal_to<>(), name))
      .value_or(false);
}

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

  static ParseError unknown(std::string_view arg);
  static ParseError value(std::string_view arg);
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
  constexpr Parser &flag(
      std::string_view short_name,
      std::string_view long_name,
      std::string_view description = ""
  ) {
    _flags.emplace_back(short_name, long_name, description);
    return *this;
  }

  constexpr Parser &
  short_flag(std::string_view short_name, std::string_view description = "") {
    _flags.emplace_back(short_name, std::nullopt, description);
    return *this;
  }

  constexpr Parser &
  long_flag(std::string_view long_name, std::string_view description = "") {
    _flags.emplace_back(std::nullopt, long_name, description);
    return *this;
  }

  constexpr Parser &option(
      std::string_view short_name,
      std::string_view long_name,
      std::string_view description = ""
  ) {
    _options.emplace_back(short_name, long_name, description);
    return *this;
  }

  constexpr Parser &
  short_option(std::string_view short_name, std::string_view description = "") {
    _options.emplace_back(short_name, std::nullopt, description);
    return *this;
  }

  constexpr Parser &
  long_option(std::string_view long_name, std::string_view description = "") {
    _options.emplace_back(std::nullopt, long_name, description);
    return *this;
  }

  constexpr Parser &program_name(std::string name) {
    _program_name = std::move(name);
    return *this;
  }

  constexpr Parser &program_description(std::string description) {
    _program_description = std::move(description);
    return *this;
  }

  [[nodiscard]] std::string generate_help(std::string_view program_name) const;

  [[nodiscard]] std::expected<Args, ParseError> parse(
      int argc, const char *const argv[] // NOLINT(modernize-avoid-c-arrays)
  ) const;

private:
  struct Entry {
    std::optional<std::string_view> short_name;
    std::optional<std::string_view> long_name;
    std::string_view description;
  };

  std::vector<Entry> _flags;
  std::vector<Entry> _options;
  std::string _program_name;
  std::string _program_description;

  [[nodiscard]] bool
  is_flag(std::string_view short_name, std::string_view long_name) const;
  [[nodiscard]] bool
  is_option(std::string_view short_name, std::string_view long_name) const;

  void set_flag(Args &args, std::string_view name) const;
  void
  set_value(Args &args, std::string_view name, std::string_view value) const;

  [[nodiscard]] static bool find_in(
      const std::vector<Entry> &vec,
      std::string_view short_name,
      std::string_view long_name
  );
  [[nodiscard]] static std::string_view
  get_store_name(std::string_view name, const std::vector<Entry> &vec);

  std::expected<void, ParseError>
  parse_long_option(ParsingContext &context, Args &args) const;
  std::expected<void, ParseError>
  parse_short_option(ParsingContext &context, Args &args) const;
  std::expected<void, ParseError>
  parse_combined_option(ParsingContext &context, Args &args) const;
};

} // namespace cli
