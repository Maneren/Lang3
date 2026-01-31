module;

#include <functional>
#include <string>

export module l3.vm:identifier;

import l3.ast;

export namespace l3::vm {

using Identifier = ast::Identifier;

} // namespace l3::vm

export template <> struct std::hash<l3::vm::Identifier> {
  std::size_t operator()(const l3::vm::Identifier &id) const {
    return std::hash<std::string>{}(id.get_name());
  }
};
