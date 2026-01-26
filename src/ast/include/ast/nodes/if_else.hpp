#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <utils/accessor.h>
#include <utils/types.h>
#include <vector>

namespace l3::ast {

class Block;
class Expression;

class IfBase {
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Block> block;

public:
  IfBase() = default;
  IfBase(Expression &&condition, Block &&block);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(condition, Expression, condition)
  DEFINE_PTR_ACCESSOR(block, Block, block)
};

class ElseIfList {
  std::vector<IfBase> inner;

public:
  ElseIfList() = default;
  ElseIfList(const ElseIfList &) = delete;
  ElseIfList(ElseIfList &&) noexcept = default;
  ElseIfList &operator=(const ElseIfList &) = delete;
  ElseIfList &operator=(ElseIfList &&) noexcept = default;
  ~ElseIfList() = default;

  ElseIfList &with_if(IfBase &&if_base);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(elseifs, std::vector<IfBase>, inner)
};

class IfElseBase {
  IfBase base_if;
  ElseIfList elseif;

public:
  IfElseBase() = default;
  IfElseBase(const IfElseBase &) = delete;
  IfElseBase(IfElseBase &&) noexcept = default;
  IfElseBase &operator=(const IfElseBase &) = delete;
  IfElseBase &operator=(IfElseBase &&) noexcept = default;
  virtual ~IfElseBase();

  IfElseBase(IfBase &&base_if);
  IfElseBase(IfBase &&base_if, ElseIfList &&elseif);

  DEFINE_ACCESSOR(base_if, IfBase, base_if)
  DEFINE_ACCESSOR(elseif, ElseIfList, elseif)
};

class IfExpression final : public IfElseBase {
  std::unique_ptr<Block> else_block;

public:
  IfExpression() = default;
  IfExpression(const IfExpression &) = delete;
  IfExpression(IfExpression &&) noexcept;
  IfExpression &operator=(const IfExpression &) = delete;
  IfExpression &operator=(IfExpression &&) noexcept;
  ~IfExpression() override;

  IfExpression(IfBase &&base_if, Block &&else_block);
  IfExpression(IfBase &&base_if, ElseIfList &&elseif, Block &&else_block);

  IfExpression &&with_elseif(IfBase &&elseif);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(else_block, Block, else_block)
};

class IfStatement final : public IfElseBase {
  std::optional<std::unique_ptr<Block>> else_block;

public:
  IfStatement() = default;
  IfStatement(const IfStatement &) = delete;
  IfStatement(IfStatement &&) noexcept;
  IfStatement &operator=(const IfStatement &) = delete;
  IfStatement &operator=(IfStatement &&) noexcept;
  ~IfStatement() override;

  IfStatement(IfBase &&base_if);
  IfStatement(IfBase &&base_if, ElseIfList &&elseif);
  IfStatement(IfBase &&base_if, ElseIfList &&elseif, Block &&else_block);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] utils::optional_cref<Block> get_else_block() const;
  [[nodiscard]] utils::optional_ref<Block> get_else_block_mut();
};

} // namespace l3::ast
