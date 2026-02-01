module;

#include <memory>
#include <utils/accessor.h>

export module l3.ast:while_loop;

export namespace l3::ast {

class Block;
class Expression;

class While {
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Block> body;

public:
  While();
  While(Expression &&condition, Block &&block);

  While(const While &) = delete;
  While(While &&) noexcept;
  While &operator=(const While &) = delete;
  While &operator=(While &&) noexcept;
  ~While();

  DEFINE_PTR_ACCESSOR_X(condition);
  DEFINE_PTR_ACCESSOR_X(body);
};

} // namespace l3::ast
