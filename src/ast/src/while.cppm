module;

#include <memory>
#include <utils/accessor.h>

export module l3.ast:while_loop;

import :block;
import :expression;

export namespace l3::ast {

class While {
  Expression condition;
  Block body;

public:
  While();
  While(Expression &&condition, Block &&block);

  While(const While &) = delete;
  While(While &&) noexcept;
  While &operator=(const While &) = delete;
  While &operator=(While &&) noexcept;
  ~While();

  DEFINE_ACCESSOR_X(condition);
  DEFINE_ACCESSOR_X(body);
};

} // namespace l3::ast
