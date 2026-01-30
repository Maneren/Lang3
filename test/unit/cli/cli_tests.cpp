#include <gtest/gtest.h>

import cli;

using namespace cli;

// Helper to create argv-style array from vector of strings
class ArgvHelper {
public:
  constexpr explicit ArgvHelper(std::initializer_list<const char *> args)
      : args_(args) {}

  [[nodiscard]] constexpr int argc() const {
    return static_cast<int>(args_.size());
  }
  constexpr const char *const *argv() { return args_.data(); }

private:
  std::vector<const char *> args_;
};

class CliParserTest : public ::testing::Test {
protected:
  Parser parser;
};

// Basic flags

TEST_F(CliParserTest, ShortFlagRecognized) {
  parser.short_flag("d");
  ArgvHelper args{"prog", "-d"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("d"));
}

TEST_F(CliParserTest, LongFlagRecognized) {
  parser.long_flag("debug");
  ArgvHelper args{"prog", "--debug"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("debug"));
}

TEST_F(CliParserTest, FlagWithBothNames) {
  parser.flag("d", "debug");
  ArgvHelper args1{"prog", "-d"};
  ArgvHelper args2{"prog", "--debug"};

  auto result1 = parser.parse(args1.argc(), args1.argv());
  auto result2 = parser.parse(args2.argc(), args2.argv());

  ASSERT_TRUE(result1.has_value());
  ASSERT_TRUE(result2.has_value());
  EXPECT_TRUE(result1->has_flag("debug"));
  EXPECT_TRUE(result2->has_flag("debug"));
}

TEST_F(CliParserTest, FlagNotPresent) {
  parser.flag("d", "debug");
  ArgvHelper args{"prog"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->has_flag("debug"));
}

TEST_F(CliParserTest, MultipleFlagsPresent) {
  parser.flag("d", "debug").long_flag("debug-lexer").long_flag("debug-parser");
  ArgvHelper args{"prog", "-d", "--debug-lexer", "--debug-parser"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("debug"));
  EXPECT_TRUE(result->has_flag("debug-lexer"));
  EXPECT_TRUE(result->has_flag("debug-parser"));
}

// Combined short flags

TEST_F(CliParserTest, CombinedShortFlags) {
  parser.short_flag("a").short_flag("b").short_flag("c");
  ArgvHelper args{"prog", "-abc"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("a"));
  EXPECT_TRUE(result->has_flag("b"));
  EXPECT_TRUE(result->has_flag("c"));
}

TEST_F(CliParserTest, CombinedFlagsWithLongNames) {
  parser.flag("a", "alpha").flag("b", "beta").flag("c", "gamma");
  ArgvHelper args{"prog", "-abc"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("alpha"));
  EXPECT_TRUE(result->has_flag("beta"));
  EXPECT_TRUE(result->has_flag("gamma"));
}

// Options

TEST_F(CliParserTest, ShortOptionWithValue) {
  parser.short_option("o");
  ArgvHelper args{"prog", "-o", "output.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("o");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "output.txt");
}

TEST_F(CliParserTest, LongOptionWithValue) {
  parser.long_option("output");
  ArgvHelper args{"prog", "--output", "file.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("output");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "file.txt");
}

TEST_F(CliParserTest, LongOptionWithEqualsValue) {
  parser.long_option("output");
  ArgvHelper args{"prog", "--output=file.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("output");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "file.txt");
}

TEST_F(CliParserTest, OptionWithBothNames) {
  parser.option("o", "output");
  ArgvHelper args{"prog", "-o", "file.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("output");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "file.txt");
}

TEST_F(CliParserTest, OptionNotPresent) {
  parser.option("o", "output");
  ArgvHelper args{"prog"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->get_value("output").has_value());
}

// Positional arguments

TEST_F(CliParserTest, SinglePositionalArgument) {
  ArgvHelper args{"prog", "input.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 1);
  EXPECT_EQ(positional[0], "input.txt");
}

TEST_F(CliParserTest, MultiplePositionalArguments) {
  ArgvHelper args{"prog", "input1.txt", "input2.txt", "input3.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 3);
  EXPECT_EQ(positional[0], "input1.txt");
  EXPECT_EQ(positional[1], "input2.txt");
  EXPECT_EQ(positional[2], "input3.txt");
}

TEST_F(CliParserTest, PositionalWithFlags) {
  parser.flag("d", "debug");
  ArgvHelper args{"prog", "-d", "input.txt", "--debug", "output.txt"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("debug"));
  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 2);
  EXPECT_EQ(positional[0], "input.txt");
  EXPECT_EQ(positional[1], "output.txt");
}

TEST_F(CliParserTest, DoubleDashSeparator) {
  parser.flag("d", "debug");
  ArgvHelper args{"prog", "-d", "--", "--not-a-flag", "-x"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("debug"));
  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 2);
  EXPECT_EQ(positional[0], "--not-a-flag");
  EXPECT_EQ(positional[1], "-x");
}

TEST_F(CliParserTest, SingleDashAsPositional) {
  ArgvHelper args{"prog", "-"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 1);
  EXPECT_EQ(positional[0], "-");
}

// Error cases

TEST_F(CliParserTest, UnknownShortFlag) {
  parser.short_flag("d");
  ArgvHelper args{"prog", "-x"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(std::string(result.error().what()).find("-x"), std::string::npos);
}

TEST_F(CliParserTest, UnknownLongFlag) {
  parser.long_flag("debug");
  ArgvHelper args{"prog", "--unknown"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(
      std::string(result.error().what()).find("--unknown"), std::string::npos
  );
}

TEST_F(CliParserTest, UnknownFlagInCombined) {
  parser.short_flag("a").short_flag("b");
  ArgvHelper args{"prog", "-abx"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(std::string(result.error().what()).find("-x"), std::string::npos);
}

TEST_F(CliParserTest, ShortOptionMissingValue) {
  parser.short_option("o");
  ArgvHelper args{"prog", "-o"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
}

TEST_F(CliParserTest, LongOptionMissingValue) {
  parser.long_option("output");
  ArgvHelper args{"prog", "--output"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
}

TEST_F(CliParserTest, LongFlagWithUnexpectedValue) {
  parser.long_flag("debug");
  ArgvHelper args{"prog", "--debug=true"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(
      std::string(result.error().what()).find("does not take a value"),
      std::string::npos
  );
}

// Complex scenarios

TEST_F(CliParserTest, RealWorldUsageScenario) {
  parser.flag("d", "debug")
      .long_flag("debug-lexer")
      .long_flag("debug-parser")
      .long_flag("debug-ast")
      .long_flag("debug-vm");
  ArgvHelper args{
      "prog", "-d", "--debug-lexer", "--debug-parser", "input.lang"
  };

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("debug"));
  EXPECT_TRUE(result->has_flag("debug-lexer"));
  EXPECT_TRUE(result->has_flag("debug-parser"));
  EXPECT_FALSE(result->has_flag("debug-ast"));
  EXPECT_FALSE(result->has_flag("debug-vm"));

  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 1);
  EXPECT_EQ(positional[0], "input.lang");
}

TEST_F(CliParserTest, MixedFlagsAndOptions) {
  parser.flag("v", "verbose").option("o", "output").option("c", "config");
  ArgvHelper args{
      "prog", "-v", "-o", "out.txt", "--config=settings.ini", "input.txt"
  };

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->has_flag("verbose"));

  auto output = result->get_value("output");
  ASSERT_TRUE(output.has_value());
  EXPECT_EQ(*output, "out.txt");

  auto config = result->get_value("config");
  ASSERT_TRUE(config.has_value());
  EXPECT_EQ(*config, "settings.ini");

  auto positional = result->positional();
  ASSERT_EQ(positional.size(), 1);
  EXPECT_EQ(positional[0], "input.txt");
}

TEST_F(CliParserTest, EmptyArguments) {
  parser.flag("d", "debug");
  ArgvHelper args{"prog"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->has_flag("debug"));
  EXPECT_TRUE(result->positional().empty());
}

TEST_F(CliParserTest, OnlyExecutableName) {
  ArgvHelper args{"prog"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->positional().empty());
}

TEST_F(CliParserTest, EqualsSignInValue) {
  parser.long_option("equation");
  ArgvHelper args{"prog", "--equation=x=y+z"};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("equation");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "x=y+z");
}

TEST_F(CliParserTest, EmptyEqualsValue) {
  parser.long_option("output");
  ArgvHelper args{"prog", "--output="};

  auto result = parser.parse(args.argc(), args.argv());

  ASSERT_TRUE(result.has_value());
  auto value = result->get_value("output");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "");
}
