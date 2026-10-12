#include <gtest/gtest.h>

#include <string>

#include "ducta/Core/SourceLocation.hpp"

using ducta::SourceLocation;

namespace {

SourceLocation capture(SourceLocation location = SourceLocation::current())
{
    return location;
}

bool endsWith(std::string const& text, std::string const& suffix)
{
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

TEST(SourceLocationTest, default_constructed_is_empty)
{
    SourceLocation location;

    EXPECT_EQ(location.line(), 0u);
    EXPECT_EQ(location.column(), 0u);
    EXPECT_STREQ(location.file_name(), "");
    EXPECT_STREQ(location.function_name(), "");
}

TEST(SourceLocationTest, constructed_from_loaded_file)
{
    SourceLocation location{12, 7, "config.yaml"};

    EXPECT_EQ(location.line(), 12u);
    EXPECT_EQ(location.column(), 7u);
    EXPECT_STREQ(location.file_name(), "config.yaml");
    EXPECT_STREQ(location.function_name(), "");
    EXPECT_TRUE(location.valid());
    EXPECT_FALSE(SourceLocation{}.valid());
}

TEST(SourceLocationTest, compares_by_content)
{
    std::string file = "config.yaml";

    EXPECT_EQ((SourceLocation{1, 2, file.c_str()}), (SourceLocation{1, 2, "config.yaml"}));
    EXPECT_NE((SourceLocation{1, 2, "a.yaml"}), (SourceLocation{1, 2, "b.yaml"}));
    EXPECT_NE((SourceLocation{1, 2}), (SourceLocation{1, 3}));
    EXPECT_EQ((SourceLocation{1, 2, nullptr}), (SourceLocation{1, 2}));
}

TEST(SourceLocationTest, current_captures_this_line)
{
    auto const line = __LINE__ + 1;
    SourceLocation location = SourceLocation::current();

    EXPECT_EQ(location.line(), static_cast<ducta::uint_least32_t>(line));
    EXPECT_TRUE(endsWith(location.file_name(), "SourceLocationTest.cpp"));
    EXPECT_NE(std::string(location.function_name()).find("TestBody"), std::string::npos);
}

TEST(SourceLocationTest, default_argument_captures_caller)
{
    auto const line = __LINE__ + 1;
    SourceLocation location = capture();

    EXPECT_EQ(location.line(), static_cast<ducta::uint_least32_t>(line));
    EXPECT_TRUE(endsWith(location.file_name(), "SourceLocationTest.cpp"));
    EXPECT_NE(std::string(location.function_name()).find("TestBody"), std::string::npos);
}

TEST(SourceLocationTest, is_usable_in_constant_expressions)
{
    constexpr SourceLocation location = SourceLocation::current();
    static_assert(location.line() == __LINE__ - 1, "line is captured at compile time");
    SUCCEED();
}
