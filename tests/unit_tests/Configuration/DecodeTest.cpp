#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/Vector.hpp"

using namespace ducta;
using namespace ducta::Configuration;

namespace {

template <class StringType>
StringView str(StringType const& s)
{
    return StringView{s.data(), s.size()};
}

struct FilterConfig
{
    String<16> name;
    double gain{1.0};
    uint8_t channels{1};
    bool enabled{true};
    Vector<int32_t, 8> taps;
    Optional<int32_t> limit;
};

struct StageConfig
{
    FilterConfig filter;
    int32_t order{0};
};

struct FilterNode
{
    using Config = FilterConfig;
};

struct PlainNode
{
};

} // namespace

DEFINE_CONFIG(
    FilterConfig,
    ("name", name),
    ("gain", gain),
    ("channels", channels),
    ("enabled", enabled),
    ("taps", taps),
    ("limit", limit));

DEFINE_CONFIG_RULES(
    FilterConfig,
    ("gain", config.gain > 0.0, "must be positive"),
    ("taps", !config.taps.empty(), "at least one tap is required"));

DEFINE_CONFIG(StageConfig, ("filter", filter), ("order", order));

// Rules that do not fit a single condition specialise ConfigRules by hand
template <>
struct ducta::Configuration::ConfigRules<StageConfig>
{
    static void validate(StageConfig const& config, Validator& validator)
    {
        if (config.order > 0 && config.filter.taps.size() < static_cast<size_t>(config.order))
        {
            validator.error("order", "must not exceed the number of taps");
        }
    }
};

namespace {

/*
 * Mimics what a loader produces for:
 *
 * 1  {
 * 2    "name": "lowpass",
 * 3    "gain": 0.5,
 * 4    "channels": 2,
 * 5    "taps": [1, 2, 3]
 * 6  }
 */
PropTree makeFilterTree()
{
    PropTree tree{SourceLocation{1, 1}};
    NodeId root = tree.root().id();
    tree.add_string(root, "name", "lowpass", {2, 13});
    tree.add_float(root, "gain", 0.5, {3, 13});
    tree.add_integer(root, "channels", 2, {4, 17});
    auto taps = tree.add_array(root, "taps", {5, 13});
    tree.add_integer(*taps, "", 1, {5, 14});
    tree.add_integer(*taps, "", 2, {5, 17});
    tree.add_integer(*taps, "", 3, {5, 20});
    return tree;
}

} // namespace

TEST(DecodeTest, decodes_valid_configuration)
{
    PropTree tree = makeFilterTree();
    Diagnostics diagnostics;

    auto config = decode<FilterConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(str(config->name), "lowpass");
    EXPECT_DOUBLE_EQ(config->gain, 0.5);
    EXPECT_EQ(config->channels, 2);
    ASSERT_EQ(config->taps.size(), 3u);
    EXPECT_EQ(config->taps[2], 3);
}

TEST(DecodeTest, missing_fields_keep_defaults)
{
    PropTree tree = makeFilterTree();
    Diagnostics diagnostics;

    auto config = decode<FilterConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_TRUE(config->enabled);
    EXPECT_FALSE(config->limit.has_value());
}

TEST(DecodeTest, optional_field_accepts_value_and_null)
{
    PropTree tree = makeFilterTree();
    tree.add_integer(tree.root().id(), "limit", 10, {6, 14});
    Diagnostics diagnostics;

    auto config = decode<FilterConfig>(tree.root(), diagnostics);
    ASSERT_TRUE(config);
    ASSERT_TRUE(config->limit.has_value());
    EXPECT_EQ(config->limit.value(), 10);

    PropTree null_tree = makeFilterTree();
    null_tree.add_null(null_tree.root().id(), "limit", {6, 14});
    auto null_config = decode<FilterConfig>(null_tree.root(), diagnostics);
    ASSERT_TRUE(null_config);
    EXPECT_FALSE(null_config->limit.has_value());
}

TEST(DecodeTest, float_field_accepts_integer)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_integer(root, "gain", 3);
    auto taps = tree.add_array(root, "taps");
    tree.add_integer(*taps, "", 1);
    Diagnostics diagnostics;

    auto config = decode<FilterConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_DOUBLE_EQ(config->gain, 3.0);
}

TEST(DecodeTest, wrong_type_is_reported_at_value_location)
{
    PropTree tree = makeFilterTree();
    tree.add_string(tree.root().id(), "enabled", "yes", {6, 16});
    Diagnostics diagnostics;

    auto config = decode<FilterConfig>(tree.root(), diagnostics);

    EXPECT_FALSE(config);
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{6, 16}));
    EXPECT_EQ(str(diagnostics[0].path), "enabled");
    EXPECT_EQ(str(diagnostics[0].message), "expected bool, got string");
}

TEST(DecodeTest, diagnostic_location_includes_source_name)
{
    PropTree tree = makeFilterTree();
    tree.set_source_name("filter.json");
    tree.add_string(tree.root().id(), "enabled", "yes", {6, 16});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<FilterConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{6, 16}));
    EXPECT_EQ(
        std::string(diagnostics[0].source.data(), diagnostics[0].source.size()), "filter.json");

    std::ostringstream out;
    out << diagnostics[0];
    EXPECT_EQ(out.str(), "filter.json:6:16: enabled: expected bool, got string");
}

TEST(DecodeTest, out_of_range_integer_is_reported)
{
    PropTree tree;
    tree.add_integer(tree.root().id(), "channels", 300, {4, 17});
    Diagnostics diagnostics;

    FilterConfig config;
    EXPECT_FALSE(decode(tree.root(), config, diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{4, 17}));
    EXPECT_EQ(str(diagnostics[0].message), "value 300 is out of range [0, 255]");
    EXPECT_EQ(config.channels, 1);
}

TEST(DecodeTest, unknown_field_is_a_warning)
{
    PropTree tree = makeFilterTree();
    tree.add_float(tree.root().id(), "gian", 2.0, {6, 3});
    Diagnostics diagnostics;

    EXPECT_TRUE(decode<FilterConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].severity, Severity::Warning);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{6, 3}));
    EXPECT_EQ(str(diagnostics[0].message), "unknown field 'gian'");
    EXPECT_FALSE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.warning_count(), 1u);
    EXPECT_EQ(diagnostics.error_count(), 0u);

    std::ostringstream out;
    out << diagnostics[0];
    EXPECT_EQ(out.str(), "6:3: gian: warning: unknown field 'gian'");
}

TEST(DecodeTest, every_unknown_field_is_reported_and_clear_resets_counts)
{
    PropTree tree = makeFilterTree();
    tree.add_float(tree.root().id(), "gian", 2.0, {6, 3});
    tree.add_string(tree.root().id(), "extra", "x", {7, 3});
    Diagnostics diagnostics;

    EXPECT_TRUE(decode<FilterConfig>(tree.root(), diagnostics));
    EXPECT_EQ(diagnostics.warning_count(), 2u);

    diagnostics.clear();
    EXPECT_EQ(diagnostics.warning_count(), 0u);
    EXPECT_TRUE(diagnostics.empty());
}

TEST(DecodeTest, configuration_must_be_an_object)
{
    PropTree tree;
    tree.add_integer(tree.root().id(), "filter", 5, {2, 13});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].path), "filter");
    EXPECT_EQ(str(diagnostics[0].message), "expected object, got integer");
}

TEST(DecodeTest, validation_error_points_to_field)
{
    // Parses and decodes fine, but the values are semantically wrong
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_float(root, "gain", -1.0, {3, 13});
    tree.add_array(root, "taps", {5, 13});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<FilterConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{3, 13}));
    EXPECT_EQ(str(diagnostics[0].path), "gain");
    EXPECT_EQ(str(diagnostics[0].message), "must be positive");
    EXPECT_EQ(diagnostics[1].location, (SourceLocation{5, 13}));
    EXPECT_EQ(str(diagnostics[1].message), "at least one tap is required");
}

TEST(DecodeTest, validation_error_records_origin_in_code)
{
    PropTree tree;
    tree.add_float(tree.root().id(), "gain", -1.0, {3, 13});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<FilterConfig>(tree.root(), diagnostics));

    ASSERT_GE(diagnostics.size(), 1u);
    SourceLocation const& origin = diagnostics[0].origin;
    EXPECT_NE(std::string(origin.file_name()).find("DecodeTest.cpp"), std::string::npos);
    EXPECT_NE(std::string(origin.function_name()).find("validate"), std::string::npos);
    EXPECT_GT(origin.line(), 0u);
}

TEST(DecodeTest, validation_of_missing_field_points_to_parent)
{
    PropTree tree;
    auto filter = tree.add_object(tree.root().id(), "filter", {2, 3});
    tree.add_float(*filter, "gain", 1.0, {3, 5});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 3}));
    EXPECT_EQ(str(diagnostics[0].path), "filter.taps");
    EXPECT_EQ(str(diagnostics[0].message), "at least one tap is required");
}

TEST(DecodeTest, collects_all_problems_with_nested_paths)
{
    PropTree tree;
    NodeId root = tree.root().id();
    auto filter = tree.add_object(root, "filter", {2, 3});
    auto taps = tree.add_array(*filter, "taps", {3, 5});
    tree.add_integer(*taps, "", 1, {3, 14});
    tree.add_string(*taps, "", "two", {3, 17});
    tree.add_bool(*filter, "channels", true, {4, 5});
    tree.add_string(root, "order", "high", {6, 3});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 3u);
    EXPECT_EQ(str(diagnostics[0].path), "filter.channels");
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{4, 5}));
    EXPECT_EQ(str(diagnostics[1].path), "filter.taps[1]");
    EXPECT_EQ(diagnostics[1].location, (SourceLocation{3, 17}));
    EXPECT_EQ(str(diagnostics[1].message), "expected integer, got string");
    EXPECT_EQ(str(diagnostics[2].path), "order");
}

TEST(DecodeTest, hand_written_rules_are_applied)
{
    PropTree tree;
    NodeId root = tree.root().id();
    auto filter = tree.add_object(root, "filter", {2, 3});
    auto taps = tree.add_array(*filter, "taps", {3, 5});
    tree.add_integer(*taps, "", 1, {3, 14});
    tree.add_integer(root, "order", 4, {5, 3});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{5, 3}));
    EXPECT_EQ(str(diagnostics[0].message), "must not exceed the number of taps");
}

TEST(DecodeTest, validator_require_reports_missing_field)
{
    PropTree tree{SourceLocation{1, 1}};
    Diagnostics diagnostics;
    Validator validator{tree.root(), diagnostics};

    EXPECT_FALSE(validator.require("name"));

    EXPECT_FALSE(validator.ok());
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{1, 1}));
    EXPECT_EQ(str(diagnostics[0].message), "missing required field 'name'");
}

TEST(DecodeTest, node_config_is_detected_and_decoded)
{
    static_assert(has_config_v<FilterNode>, "FilterNode declares a Config");
    static_assert(!has_config_v<PlainNode>, "PlainNode has no Config");
    static_assert(is_same_v<config_t<FilterNode>, FilterConfig>, "Config type");

    PropTree tree = makeFilterTree();
    Diagnostics diagnostics;

    auto config = decode_node_config<FilterNode>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_EQ(str(config->name), "lowpass");
}

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

TEST(DecodeTest, embedded_capacity_overflow_is_reported)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_string(root, "name", "a_name_longer_than_16", {2, 13});
    auto taps = tree.add_array(root, "taps", {5, 13});
    for (int i = 0; i < 9; ++i)
    {
        tree.add_integer(*taps, "", i);
    }
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<FilterConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(str(diagnostics[0].message), "string is too long (21 characters, max 16)");
    EXPECT_EQ(str(diagnostics[1].message), "too many elements (9, max 8)");
}

TEST(DecodeTest, embedded_tree_limits_are_enforced)
{
    PropTree tree;
    NodeId root = tree.root().id();

    std::string long_value(DUCTA_CONFIG_MAX_STRING_LENGTH + 1, 'x');
    EXPECT_FALSE(tree.add_string(root, "value", StringView{long_value.data(), long_value.size()}));

    std::string long_key(DUCTA_CONFIG_MAX_KEY_LENGTH + 1, 'k');
    EXPECT_FALSE(tree.add_integer(root, StringView{long_key.data(), long_key.size()}, 1));

    auto list = tree.add_array(root, "list");
    ASSERT_TRUE(list);
    while (tree.size() < DUCTA_CONFIG_MAX_NODES)
    {
        ASSERT_TRUE(tree.add_integer(*list, "", 0));
    }
    EXPECT_FALSE(tree.add_integer(*list, "", 0));
}

TEST(DecodeTest, embedded_long_source_name_keeps_its_end)
{
    PropTree tree;
    std::string name(DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH, 'd');
    name += "/filter.json";

    tree.set_source_name(StringView{name.data(), name.size()});

    StringView stored = tree.source_name();
    EXPECT_EQ(stored.size(), static_cast<size_t>(DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH));
    EXPECT_EQ(stored.substr(stored.size() - 12), "/filter.json");
}

#endif
