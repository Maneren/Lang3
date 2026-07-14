export module l3.ast:while_loop;

import :block;
import :expression;
import std;
import l3.location;

export namespace l3::ast {

class While {
  Expression condition;
  Block body;

  DEFINE_LOCATION_FIELD()

public:
  While(location::Location location = {});
  While(
      Expression &&condition, Block &&block, location::Location location = {}
  );

  While(const While &) = delete;
  While(While &&) noexcept;
  While &operator=(const While &) = delete;
  While &operator=(While &&) noexcept;
  ~While();

  DEFINE_ACCESSOR_X(condition);
  DEFINE_ACCESSOR_X(body);
};

} // namespace l3::ast
