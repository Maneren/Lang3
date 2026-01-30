module;

#include <memory>

module l3.ast;

import :block;
import :name_list;

namespace l3::ast {

FunctionBody::FunctionBody(NameList &&parameters, Block &&block)
    : parameters(std::move(parameters)),
      block(std::make_unique<Block>(std::move(block))) {};

FunctionBody::FunctionBody(FunctionBody &&) noexcept = default;
FunctionBody &FunctionBody::operator=(FunctionBody &&) noexcept = default;
FunctionBody::~FunctionBody() = default;

} // namespace l3::ast
