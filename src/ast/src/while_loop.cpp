module l3.ast;

namespace l3::ast {

While::While(location::Location location) : location_(location) {}
While::While(Expression &&condition, Block &&block, location::Location location)
    : condition(std::move(condition)), body(std::move(block)),
      location_(location) {}

While::While(While &&) noexcept = default;
While &While::operator=(While &&) noexcept = default;
While::~While() = default;

} // namespace l3::ast
