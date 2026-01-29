module;

#include <format>

export module l3.ast:format;

export template <typename Node>
  requires requires(
      Node const &node, std::back_insert_iterator<std::string> &out
  ) {
    { node.print(out) } -> std::same_as<void>;
  }
struct std::formatter<Node> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static constexpr auto format(Node const &node, std::format_context &ctx) {
    auto out = ctx.out();
    node.print(out);
    return out;
  }
};
