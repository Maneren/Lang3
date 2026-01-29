module;

#include <iterator>
#include <memory>
#include <utils/accessor.h>

export module l3.ast:while_loop;

import :expression;

export namespace l3::ast {

class Block;

class While {
  Expression condition;
  std::unique_ptr<Block> body;

public:
  While();
  While(Expression &&condition, Block &&block);

  While(const While &) = delete;
  While(While &&) noexcept;
  While &operator=(const While &) = delete;
  While &operator=(While &&) noexcept;
  ~While();

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(condition, Expression, condition);
  DEFINE_PTR_ACCESSOR(body, Block, body);
};

} // namespace l3::ast
