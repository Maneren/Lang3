#pragma once

#include "ast/nodes/block.hpp"
#include <memory>
#include <optional>
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

  void print(std::output_iterator<char> auto &out, size_t depth) const;

  [[nodiscard]] const Expression &get_condition() const { return *condition; }
  [[nodiscard]] const Block &get_block() const { return *block; }
};

using ElseIfList = std::vector<IfBase>;

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

  [[nodiscard]] const IfBase &get_base_if() const { return base_if; }
  [[nodiscard]] const ElseIfList &get_elseif() const { return elseif; }
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
  IfExpression(
      IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
  );

  IfExpression &&with_elseif(IfBase &&elseif);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
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
  IfStatement(IfBase &&base_if, std::vector<IfBase> &&elseif);
  IfStatement(
      IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
  );

  IfStatement &&with_elseif(IfBase &&elseif);

  void print(std::output_iterator<char> auto &out, size_t depth) const;

  [[nodiscard]] std::optional<std::reference_wrapper<const Block>>
  get_else_block() const;
};

} // namespace l3::ast
