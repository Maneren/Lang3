#pragma once

#include <functional>
#include <string>

import l3.ast;

namespace l3::vm {

using Identifier = ast::Identifier;

}

template <> struct std::hash<l3::vm::Identifier> {
  std::size_t operator()(const l3::vm::Identifier &id) const {
    return std::hash<std::string>{}(id.get_name());
  }
};
