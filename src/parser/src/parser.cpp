#include <parser.tab.hpp>

namespace l3::parser {

void L3Parser::error(const location_type &loc, const std::string &msg) {
  std::cerr << "Error at " << loc << ": " << msg << '\n';
}

} // namespace l3::parser
