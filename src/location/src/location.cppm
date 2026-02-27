export module l3.location;

import utils;
import std;

export namespace l3::location {

// Adapted from the Bison C++ skeleton

struct Position {
  using filename_type = std::string_view;
  using counter_type = std::size_t;

  explicit Position(
      std::optional<filename_type> f = std::nullopt,
      counter_type l = 1,
      counter_type c = 1
  )
      : filename(f), line(l), column(c) {}

  void initialize(filename_type fn, counter_type l = 1, counter_type c = 1) {
    filename = fn;
    line = l;
    column = c;
  }

  void lines(counter_type count = 1) {
    if (count) {
      column = 1;
      line = add_(line, count, 1);
    }
  }

  void columns(counter_type count = 1) { column = add_(column, count, 1); }

  [[nodiscard]] constexpr std::strong_ordering
  operator<=>(const Position &) const = default;

  std::optional<filename_type> filename;
  counter_type line;
  counter_type column;

private:
  static counter_type
  add_(counter_type lhs, counter_type rhs, counter_type min) {
    return lhs + rhs < min ? min : lhs + rhs;
  }
};

inline Position &operator+=(Position &res, Position::counter_type width) {
  res.columns(width);
  return res;
}

inline Position operator+(Position res, Position::counter_type width) {
  return res += width;
}

inline Position &operator-=(Position &res, Position::counter_type width) {
  return res += -width;
}

inline Position operator-(Position res, Position::counter_type width) {
  return res -= width;
}

template <typename Char>
std::basic_ostream<Char> &
operator<<(std::basic_ostream<Char> &ostr, const Position &pos) {
  std::print(ostr, "{}:{}:{}", pos.filename, pos.line, pos.column);
  return ostr;
}

struct Location {
  using filename_type = Position::filename_type;
  using counter_type = Position::counter_type;

  Location(const Position &b, const Position &e) : begin(b), end(e) {}
  Location(const Position &p = Position{}) : begin(p), end(p) {}

  void initialize(filename_type f, counter_type l = 1, counter_type c = 1) {
    begin.initialize(f, l, c);
    end = begin;
  }

  void step() { begin = end; }
  void columns(counter_type count = 1) { end += count; }
  void lines(counter_type count = 1) { end.lines(count); }

  [[nodiscard]] constexpr std::strong_ordering
  operator<=>(const Location &) const = default;

  Position begin;
  Position end;
};

inline Location &operator+=(Location &res, const Location &end) {
  res.end = end.end;
  return res;
}

inline Location operator+(Location res, const Location &end) {
  return res += end;
}

inline Location &operator+=(Location &res, Location::counter_type width) {
  res.columns(width);
  return res;
}

inline Location operator+(Location res, Location::counter_type width) {
  return res += width;
}

inline Location &operator-=(Location &res, Location::counter_type width) {
  return res += -width;
}

inline Location operator-(Location res, Location::counter_type width) {
  return res -= width;
}

template <typename Char>
std::basic_ostream<Char> &
operator<<(std::basic_ostream<Char> &ostr, const Location &loc) {
  std::print(ostr, "{}", loc);
  return ostr;
}

} // namespace l3::location

export template <>
struct std::formatter<l3::location::Position>
    : utils::static_formatter<l3::location::Position> {
  static constexpr auto format(const auto &pos, std::format_context &ctx) {
    if (pos.filename) {
      return std::format_to(
          ctx.out(), "{}:{}.{}", *pos.filename, pos.line, pos.column
      );
    }
    return std::format_to(ctx.out(), "{}.{}", pos.line, pos.column);
  }
};

export template <>
struct std::formatter<l3::location::Location>
    : utils::static_formatter<l3::location::Location> {
  static constexpr auto format(const auto &loc, std::format_context &ctx) {
    // file.l3:10.5
    auto out = std::format_to(ctx.out(), "{}", loc.begin);
    if (loc.begin == loc.end) {
      return out;
    }
    if (loc.begin.line != loc.end.line) {
      // Multi-line: file.l3:10.5-15.20
      return std::format_to(ctx.out(), "-{}.{}", loc.end.line, loc.end.column);
    }
    // Same line: file.l3:10.5-20
    return std::format_to(ctx.out(), "-{}", loc.end.column);
  }
};
