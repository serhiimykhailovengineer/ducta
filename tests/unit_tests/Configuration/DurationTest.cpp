#include <gtest/gtest.h>

#include <string>

#include "ducta/Configuration/Configuration.hpp"

using namespace ducta;
using namespace ducta::Configuration;

namespace {

using Microseconds = Chrono::TimestampUS;

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <intmax_t Num, intmax_t Den>
using Ratio = etl::ratio<Num, Den>;
#else
template <intmax_t Num, intmax_t Den>
using Ratio = std::ratio<Num, Den>;
#endif
using Milliseconds = Chrono::chrono::milliseconds;
using Nanoseconds = Chrono::chrono::nanoseconds;

template <class StringType>
std::string str(StringType const& s)
{
    return std::string{s.data(), s.size()};
}

/* Decodes the "value" member of { "value": <json> } */
template <class Duration>
bool decodeDuration(char const* json, Duration& out, Diagnostics& diagnostics)
{
    std::string const text = std::string{"{ \"value\": "} + json + " }";
    PropTree tree;
    EXPECT_TRUE(loadJson(StringView{text.data(), text.size()}, tree, diagnostics)) << json;
    return decode(tree.root().child("value"), out, diagnostics);
}

template <class Duration = Microseconds>
Duration ok(char const* json)
{
    Diagnostics diagnostics;
    Duration value{-12345};
    EXPECT_TRUE(decodeDuration(json, value, diagnostics)) << json;
    EXPECT_TRUE(diagnostics.empty()) << json;
    return value;
}

template <class Duration = Microseconds>
std::string error(char const* json)
{
    Diagnostics diagnostics;
    Duration value{};
    EXPECT_FALSE(decodeDuration(json, value, diagnostics)) << json;
    EXPECT_EQ(diagnostics.size(), 1u) << json;
    return diagnostics.size() > 0 ? str(diagnostics[0].message) : std::string{};
}

} // namespace

TEST(DurationTest, units)
{
    EXPECT_EQ(ok(R"("1000ns")"), Microseconds{1});
    EXPECT_EQ(ok(R"("250us")"), Microseconds{250});
    EXPECT_EQ(ok("\"250\xC2\xB5s\""), Microseconds{250});
    EXPECT_EQ(ok(R"("10ms")"), Microseconds{10000});
    EXPECT_EQ(ok(R"("2s")"), Microseconds{2000000});
    EXPECT_EQ(ok(R"("3m")"), Microseconds{180000000});
    EXPECT_EQ(ok(R"("3min")"), Microseconds{180000000});
    EXPECT_EQ(ok(R"("1h")"), Microseconds{3600000000LL});
    EXPECT_EQ(ok(R"("1d")"), Microseconds{86400000000LL});
}

TEST(DurationTest, fractions_spaces_signs_and_compound)
{
    EXPECT_EQ(ok(R"("1.5s")"), Microseconds{1500000});
    EXPECT_EQ(ok(R"(".5ms")"), Microseconds{500});
    EXPECT_EQ(ok(R"("2.ms")"), Microseconds{2000});
    EXPECT_EQ(ok(R"(" 10 ms ")"), Microseconds{10000});
    EXPECT_EQ(ok(R"("1h30m")"), Microseconds{5400000000LL});
    EXPECT_EQ(ok(R"("1m 30s 250ms")"), Microseconds{90250000});
    EXPECT_EQ(ok(R"("-5ms")"), Microseconds{-5000});
    EXPECT_EQ(ok(R"("+5ms")"), Microseconds{5000});
    EXPECT_EQ(ok(R"("0s")"), Microseconds{0});
}

TEST(DurationTest, integer_is_a_count_of_ticks)
{
    EXPECT_EQ(ok("500"), Microseconds{500});
    EXPECT_EQ(ok<Milliseconds>("500"), Milliseconds{500});
}

TEST(DurationTest, other_destination_types)
{
    EXPECT_EQ(ok<Milliseconds>(R"("2s")"), Milliseconds{2000});
    EXPECT_EQ(ok<Nanoseconds>(R"("1.5us")"), Nanoseconds{1500});
}

TEST(DurationTest, fractional_nanosecond_periods)
{
    // A tick of 2/3 ns: "2ns" is exactly 3 ticks, "1ns" is not a whole number of ticks
    using TwoThirdsNs = Chrono::chrono::duration<int64_t, Ratio<1, 1500000000>>;
    EXPECT_EQ(ok<TwoThirdsNs>(R"("2ns")"), TwoThirdsNs{3});
    EXPECT_EQ(ok<TwoThirdsNs>(R"("1us")"), TwoThirdsNs{1500});
    Diagnostics diagnostics;
    TwoThirdsNs value{};
    EXPECT_FALSE(decodeDuration(R"("1ns")", value, diagnostics));

    // Picoseconds: finer than a nanosecond
    using Picoseconds = Chrono::chrono::duration<int64_t, Ratio<1, 1000000000000>>;
    EXPECT_EQ(ok<Picoseconds>(R"("3ns")"), Picoseconds{3000});
}

TEST(DurationTest, precision_below_tick_is_an_error)
{
    EXPECT_EQ(error(R"("1.5us")"), "duration '1.5us' is not a whole number of 1us");
    EXPECT_EQ(error(R"("10ns")"), "duration '10ns' is not a whole number of 1us");
    EXPECT_EQ(error<Milliseconds>(R"("10us")"), "duration '10us' is not a whole number of 1ms");
    EXPECT_EQ(error(R"("0.5ns")"), "duration '0.5ns' is not a whole number of 1us");
}

TEST(DurationTest, syntax_errors)
{
    EXPECT_EQ(
        error(R"("10")"), "invalid duration '10' (expected e.g. \"10ms\", \"1.5s\", \"1h30m\")");
    EXPECT_EQ(error(R"("")"), "invalid duration '' (expected e.g. \"10ms\", \"1.5s\", \"1h30m\")");
    EXPECT_EQ(
        error(R"("ms")"), "invalid duration 'ms' (expected e.g. \"10ms\", \"1.5s\", \"1h30m\")");
    EXPECT_EQ(
        error(R"("1.s.5")"),
        "invalid duration '1.s.5' (expected e.g. \"10ms\", \"1.5s\", \"1h30m\")");
    EXPECT_EQ(
        error(R"("5 parsecs")"),
        "unknown duration unit 'parsecs' (expected ns, us, ms, s, m, h, d)");
}

TEST(DurationTest, range_errors)
{
    EXPECT_EQ(error(R"("999999999999d")"), "duration '999999999999d' is out of range");
    EXPECT_EQ(
        error(R"("99999999999999999999ms")"), "duration '99999999999999999999ms' is out of range");
}

TEST(DurationTest, wrong_kind)
{
    EXPECT_EQ(error("1.5"), "expected a duration string (e.g. \"10ms\") or integer, got float");
    EXPECT_EQ(error("true"), "expected a duration string (e.g. \"10ms\") or integer, got bool");
}

TEST(DurationTest, error_location_points_to_value)
{
    PropTree tree;
    Diagnostics diagnostics;
    ASSERT_TRUE(loadJson("{\n  \"timeout\": \"10 parsecs\"\n}", tree, diagnostics));

    Microseconds value{};
    EXPECT_FALSE(decode(tree.root().child("timeout"), value, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location.line(), 2u);
    EXPECT_EQ(str(diagnostics[0].path), "timeout");
}
