#pragma once

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
};

using IfElseList = std::vector<IfBase>;

class IfExpression final {
  IfBase base_if;
  std::vector<IfBase> elseif;
  std::unique_ptr<Block> else_block;

public:
  IfExpression() = default;
  IfExpression(const IfExpression &) = delete;
  IfExpression(IfExpression &&) noexcept;
  IfExpression &operator=(const IfExpression &) = delete;
  IfExpression &operator=(IfExpression &&) noexcept;
  ~IfExpression();

  IfExpression(IfBase &&base_if, Block &&else_block);
  IfExpression(
      IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
  );

  IfExpression &&with_elseif(IfBase &&elseif);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class IfStatement final {
  IfBase base_if;
  std::vector<IfBase> elseif;
  std::optional<std::unique_ptr<Block>> else_block;

public:
  IfStatement() = default;
  IfStatement(const IfStatement &) = delete;
  IfStatement(IfStatement &&) noexcept;
  IfStatement &operator=(const IfStatement &) = delete;
  IfStatement &operator=(IfStatement &&) noexcept;
  ~IfStatement();

  IfStatement(IfBase &&base_if);
  IfStatement(IfBase &&base_if, std::vector<IfBase> &&elseif);
  IfStatement(
      IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
  );

  IfStatement &&with_elseif(IfBase &&elseif);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

} // namespace l3::ast
