#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "ducta/Configuration/Configuration.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <cstdio>
#include <fstream>
#endif

using namespace ducta;
using namespace ducta::Configuration;

namespace {

template <class StringType>
StringView str(StringType const& s)
{
    return StringView{s.data(), s.size()};
}

/*
 * Loads text expecting exactly one diagnostic. The file name of the returned location is
 * dropped: it points into the tree, which does not outlive this function.
 */
Diagnostic loadError(char const* text, JsonOptions options = {})
{
    PropTree tree;
    Diagnostics diagnostics;
    EXPECT_FALSE(loadJson(text, tree, diagnostics, options)) << text;
    EXPECT_EQ(diagnostics.size(), 1u) << text;
    if (diagnostics.size() == 0)
    {
        return Diagnostic{};
    }
    Diagnostic error = diagnostics[0];
    error.location = SourceLocation{error.location.line(), error.location.column()};
    return error;
}

struct GainConfig
{
    double gain{1.0};
    int32_t channels{1};
};

} // namespace

DEFINE_CONFIG(GainConfig, ("gain", gain), ("channels", channels));
DEFINE_CONFIG_RULES(GainConfig, ("gain", config.gain > 0.0, "must be positive"));

TEST(JsonTest, loads_values_with_locations)
{
    char const* text = R"({
  "name": "mixer",
  "gain": 0.5,
  "channels": 2,
  "enabled": true,
  "limit": null,
  "taps": [1, -2, 3e2],
  "filter": { "type": "lowpass" }
})";
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson(text, tree, diagnostics));
    EXPECT_TRUE(diagnostics.empty());

    PropNode root = tree.root();
    EXPECT_EQ(root.location(), (SourceLocation{1, 1}));
    EXPECT_EQ(root.size(), 7u);

    EXPECT_EQ(str(root.child("name").as_string()), "mixer");
    EXPECT_EQ(root.child("name").location(), (SourceLocation{2, 11}));
    EXPECT_DOUBLE_EQ(root.child("gain").as_float(), 0.5);
    EXPECT_EQ(root.child("gain").location(), (SourceLocation{3, 11}));
    EXPECT_EQ(root.child("channels").as_integer(), 2);
    EXPECT_EQ(root.child("channels").location(), (SourceLocation{4, 15}));
    EXPECT_TRUE(root.child("enabled").as_bool());
    EXPECT_EQ(root.child("enabled").location(), (SourceLocation{5, 14}));
    EXPECT_TRUE(root.child("limit").is_null());
    EXPECT_EQ(root.child("limit").location(), (SourceLocation{6, 12}));

    PropNode taps = root.child("taps");
    ASSERT_TRUE(taps.is_array());
    EXPECT_EQ(taps.location(), (SourceLocation{7, 11}));
    ASSERT_EQ(taps.size(), 3u);
    EXPECT_EQ(taps.at(0).as_integer(), 1);
    EXPECT_EQ(taps.at(1).as_integer(), -2);
    EXPECT_EQ(taps.at(1).location(), (SourceLocation{7, 15}));
    EXPECT_EQ(taps.at(2).kind(), NodeKind::Float);
    EXPECT_DOUBLE_EQ(taps.at(2).as_float(), 300.0);
    EXPECT_EQ(taps.at(2).location(), (SourceLocation{7, 19}));

    PropNode filter = root.child("filter");
    ASSERT_TRUE(filter.is_object());
    EXPECT_EQ(filter.location(), (SourceLocation{8, 13}));
    EXPECT_EQ(str(filter.child("type").as_string()), "lowpass");
    EXPECT_EQ(filter.child("type").location(), (SourceLocation{8, 23}));
}

TEST(JsonTest, keeps_member_order_and_empty_containers)
{
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson(R"({"z": {}, "a": [], "m": [[], {}]})", tree, diagnostics));

    EXPECT_EQ(str(tree.root().at(0).key()), "z");
    EXPECT_EQ(str(tree.root().at(1).key()), "a");
    EXPECT_TRUE(tree.root().child("z").is_object());
    EXPECT_EQ(tree.root().child("z").size(), 0u);
    EXPECT_TRUE(tree.root().child("a").is_array());
    EXPECT_EQ(tree.root().child("m").size(), 2u);
}

TEST(JsonTest, decodes_string_escapes)
{
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson(
        R"({"plain": "a\"b\\c\/d\n\t\r\b\f", "bmp": "é中", "pair": "😀", "key!": 1})", tree, diagnostics));

    EXPECT_EQ(str(tree.root().child("plain").as_string()), "a\"b\\c/d\n\t\r\b\f");
    EXPECT_EQ(str(tree.root().child("bmp").as_string()), "\xC3\xA9\xE4\xB8\xAD");
    EXPECT_EQ(str(tree.root().child("pair").as_string()), "\xF0\x9F\x98\x80");
    EXPECT_TRUE(tree.root().child("key!").valid());
}

TEST(JsonTest, columns_count_utf8_characters)
{
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson("{\"\xC3\xA9\": \"\xC3\xBC\", \"x\": 1}", tree, diagnostics));

    EXPECT_EQ(tree.root().child("x").location(), (SourceLocation{1, 17}));
}

TEST(JsonTest, handles_crlf_and_byte_order_mark)
{
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson("\xEF\xBB\xBF{\r\n  \"a\": 1\r\n}", tree, diagnostics));

    EXPECT_EQ(tree.root().location(), (SourceLocation{1, 1}));
    EXPECT_EQ(tree.root().child("a").location(), (SourceLocation{2, 8}));
}

TEST(JsonTest, parses_numbers)
{
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson(
        R"({"max": 9223372036854775807, "min": -9223372036854775808, "zero": 0, "neg": -0.5, "exp": 1.5E-3})",
        tree,
        diagnostics));

    EXPECT_EQ(tree.root().child("max").as_integer(), INT64_MAX);
    EXPECT_EQ(tree.root().child("min").as_integer(), INT64_MIN);
    EXPECT_EQ(tree.root().child("zero").as_integer(), 0);
    EXPECT_DOUBLE_EQ(tree.root().child("neg").as_float(), -0.5);
    EXPECT_DOUBLE_EQ(tree.root().child("exp").as_float(), 0.0015);
}

TEST(JsonTest, out_of_range_numbers_are_reported_and_loading_continues)
{
    PropTree tree;
    Diagnostics diagnostics;

    EXPECT_FALSE(
        loadJson(R"({"big": 9223372036854775808, "huge": 1e400, "ok": 1})", tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{1, 9}));
    EXPECT_EQ(str(diagnostics[0].path), "big");
    EXPECT_EQ(str(diagnostics[0].message), "integer 9223372036854775808 is out of range");
    EXPECT_EQ(str(diagnostics[1].path), "huge");
    EXPECT_EQ(str(diagnostics[1].message), "number 1e400 is out of range");
    EXPECT_EQ(tree.root().child("ok").as_integer(), 1);
}

TEST(JsonTest, invalid_literals_and_numbers_are_syntax_errors)
{
    Diagnostic error = loadError(R"({"n": 01})");
    EXPECT_EQ(error.location, (SourceLocation{1, 7}));
    EXPECT_EQ(str(error.message), "invalid value '01'");

    EXPECT_EQ(str(loadError(R"({"n": 1.})").message), "invalid value '1.'");
    EXPECT_EQ(str(loadError(R"({"n": -})").message), "invalid value '-'");
    EXPECT_EQ(str(loadError(R"({"n": tru})").message), "invalid value 'tru'");
    EXPECT_EQ(str(loadError(R"({"n": .5})").message), "unexpected character '.'");
}

TEST(JsonTest, structural_errors_are_located)
{
    Diagnostic error = loadError(R"({"a": 1 "b": 2})");
    EXPECT_EQ(error.location, (SourceLocation{1, 9}));
    EXPECT_EQ(str(error.message), "expected ',' or '}'");

    error = loadError(R"({"a": [1 2]})");
    EXPECT_EQ(error.location, (SourceLocation{1, 10}));
    EXPECT_EQ(str(error.message), "expected ','");

    error = loadError(R"({"a": [1, 2,]})");
    EXPECT_EQ(error.location, (SourceLocation{1, 12}));
    EXPECT_EQ(str(error.message), "unexpected ',' before ']'");

    error = loadError(R"({"a": 1,})");
    EXPECT_EQ(str(error.message), "unexpected ',' before '}'");

    error = loadError(R"({"a" 1})");
    EXPECT_EQ(error.location, (SourceLocation{1, 6}));
    EXPECT_EQ(str(error.message), "unexpected character '1'");

    error = loadError(R"({"a"})");
    EXPECT_EQ(str(error.message), "expected ':' followed by a value");

    error = loadError(R"({"a": [1}})");
    EXPECT_EQ(error.location, (SourceLocation{1, 9}));
    EXPECT_EQ(str(error.message), "unexpected character '}'");

    error = loadError(R"({,"a": 1})");
    EXPECT_EQ(error.location, (SourceLocation{1, 2}));
    EXPECT_EQ(str(error.message), "unexpected character ','");
}

TEST(JsonTest, incomplete_input_is_reported)
{
    Diagnostic error = loadError(R"({"a": "abc})");
    EXPECT_EQ(error.location, (SourceLocation{1, 7}));
    EXPECT_EQ(str(error.message), "unterminated string");

    error = loadError("{\"a\": [1, 2\n");
    EXPECT_EQ(error.location, (SourceLocation{2, 1}));
    EXPECT_EQ(str(error.message), "unexpected end of input");

    EXPECT_EQ(str(loadError("").message), "expected a JSON object");
    EXPECT_EQ(str(loadError("  \n ").message), "expected a JSON object");
}

TEST(JsonTest, top_level_must_be_a_single_object)
{
    EXPECT_EQ(str(loadError("[1, 2]").message), "expected an object at the top level, got array");

    Diagnostic error = loadError(R"({"a": 1} {})");
    EXPECT_EQ(error.location, (SourceLocation{1, 10}));
    EXPECT_EQ(str(error.message), "unexpected data after the top-level object");

    error = loadError(R"({"a": 1} x)");
    EXPECT_EQ(error.location, (SourceLocation{1, 10}));
    EXPECT_EQ(str(error.message), "unexpected character 'x'");
}

TEST(JsonTest, invalid_strings_are_reported)
{
    Diagnostic error = loadError("{\"a\": \"x\ty\"}");
    EXPECT_EQ(error.location, (SourceLocation{1, 9}));
    EXPECT_EQ(str(error.message), "control character in string must be escaped");

    EXPECT_EQ(str(loadError(R"({"a": "\q"})").message), "invalid escape sequence in string");
    EXPECT_EQ(
        str(loadError(R"({"a": "\ud800"})").message),
        "invalid unicode escape in string (unpaired surrogate)");
    EXPECT_EQ(
        str(loadError(R"({"a": "\udc00x"})").message),
        "invalid unicode escape in string (unpaired surrogate)");
}

TEST(JsonTest, duplicate_keys_are_reported)
{
    PropTree tree;
    Diagnostics diagnostics;

    EXPECT_FALSE(loadJson("{\n  \"a\": 1,\n  \"a\": {\"x\": 2}\n}", tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{3, 3}));
    EXPECT_EQ(str(diagnostics[0].path), "a");
    EXPECT_EQ(str(diagnostics[0].message), "duplicate key 'a' (first defined at 2:8)");
    EXPECT_EQ(tree.root().child("a").as_integer(), 1);
}

TEST(JsonTest, nesting_depth_is_limited)
{
    JsonOptions options;
    options.max_depth = 2;

    PropTree tree;
    Diagnostics diagnostics;
    EXPECT_TRUE(loadJson(R"({"a": {"b": 1}})", tree, diagnostics, options));

    Diagnostic error = loadError(R"({"a": {"b": [1]}})", options);
    EXPECT_EQ(error.location, (SourceLocation{1, 13}));
    EXPECT_EQ(str(error.message), "nesting is too deep (max 2)");
}

TEST(JsonTest, replaces_tree_content_and_keeps_source_name)
{
    PropTree tree;
    tree.set_source_name("pipeline.json");
    tree.add_integer(tree.root().id(), "old", 1);
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJson(R"({"new": 2})", tree, diagnostics));

    EXPECT_FALSE(tree.root().child("old").valid());
    EXPECT_EQ(str(tree.source_name()), "pipeline.json");
    EXPECT_EQ(tree.root().child("new").location(), (SourceLocation{1, 9, "pipeline.json"}));
}

TEST(JsonTest, syntax_error_location_includes_source_name)
{
    PropTree tree;
    tree.set_source_name("broken.json");
    Diagnostics diagnostics;

    EXPECT_FALSE(loadJson(R"({"a": [1 2]})", tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{1, 10, "broken.json"}));
}

TEST(JsonTest, decode_errors_point_into_json_text)
{
    char const* text = R"({
  "gain": -1,
  "channels": "two"
})";
    PropTree tree;
    tree.set_source_name("gain.json");
    Diagnostics diagnostics;
    ASSERT_TRUE(loadJson(text, tree, diagnostics));

    EXPECT_FALSE(decode<GainConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    std::ostringstream out;
    out << diagnostics[0];
    EXPECT_EQ(out.str(), "gain.json:3:15: channels: expected integer, got string");

    PropTree valid_types;
    valid_types.set_source_name("gain.json");
    Diagnostics rule_diagnostics;
    ASSERT_TRUE(loadJson("{\n  \"gain\": -1\n}", valid_types, rule_diagnostics));
    EXPECT_FALSE(decode<GainConfig>(valid_types.root(), rule_diagnostics));
    ASSERT_EQ(rule_diagnostics.size(), 1u);
    std::ostringstream rule_out;
    rule_out << rule_diagnostics[0];
    EXPECT_EQ(rule_out.str(), "gain.json:2:11: gain: must be positive");
}

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)

TEST(JsonTest, loads_file_and_uses_path_as_source_name)
{
    std::string const path = ::testing::TempDir() + "ducta_json_test.json";
    {
        std::ofstream file{path, std::ios::binary};
        file << "{\n  \"gain\": 2.5\n}\n";
    }
    PropTree tree;
    Diagnostics diagnostics;

    ASSERT_TRUE(loadJsonFile(StringView{path.data(), path.size()}, tree, diagnostics));

    EXPECT_EQ(str(tree.source_name()), (StringView{path.data(), path.size()}));
    EXPECT_EQ(tree.root().child("gain").location(), (SourceLocation{2, 11, path.c_str()}));
    std::remove(path.c_str());
}

TEST(JsonTest, missing_file_is_reported)
{
    PropTree tree;
    Diagnostics diagnostics;

    EXPECT_FALSE(loadJsonFile("/nonexistent/ducta.json", tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "cannot open file '/nonexistent/ducta.json'");
}

#else

TEST(JsonTest, embedded_limits_are_reported)
{
    PropTree tree;
    Diagnostics diagnostics;

    std::string long_string(DUCTA_CONFIG_MAX_STRING_LENGTH + 1, 'x');
    std::string long_key(DUCTA_CONFIG_MAX_KEY_LENGTH + 1, 'k');
    std::string text = "{\"s\": \"" + long_string + "\", \"" + long_key + "\": 1, \"ok\": 2}";

    EXPECT_FALSE(loadJson(StringView{text.data(), text.size()}, tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(str(diagnostics[0].path), "s");
    EXPECT_EQ(str(diagnostics[0].message), "string is too long (max 64 characters)");
    EXPECT_EQ(str(diagnostics[1].message), "string is too long (max 32 characters)");
    EXPECT_EQ(tree.root().child("ok").as_integer(), 2);
}

TEST(JsonTest, embedded_token_limit_is_reported)
{
    std::string text = "{\"a\": [";
    for (int i = 0; i < DUCTA_CONFIG_MAX_JSON_TOKENS; ++i)
    {
        text += i == 0 ? "0" : ",0";
    }
    text += "]}";
    PropTree tree;
    Diagnostics diagnostics;

    EXPECT_FALSE(loadJson(StringView{text.data(), text.size()}, tree, diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_NE(
        std::string(diagnostics[0].message.data(), diagnostics[0].message.size()).find("too many JSON tokens"),
        std::string::npos);
}

#endif
