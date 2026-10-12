#include <gtest/gtest.h>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/Variant.hpp"
#include "ducta/Core/Types/Vector.hpp"

using namespace ducta;
using namespace ducta::Configuration;

namespace {

template <class StringType>
StringView str(StringType const& s)
{
    return StringView{s.data(), s.size()};
}

using Name = String<16>;

struct ValueConfig
{
    Variant<int32_t, Name> value;
    Variant<double, bool> number;
    Optional<Variant<int32_t, Name>> maybe;
    Vector<Variant<int32_t, Name>, 4> items;
};

struct GainConfig
{
    double gain{1.0};
};

struct SourceConfig
{
    Variant<int32_t, GainConfig> source;
};

struct LowPassConfig
{
    double cutoff{100.0};
    int32_t order{2};
};

struct HighPassConfig
{
    double cutoff{1000.0};
};

using FilterVariant = Variant<LowPassConfig, HighPassConfig>;

struct StageConfig
{
    FilterVariant filter;
    Name name;
};

} // namespace

DEFINE_CONFIG(ValueConfig, ("value", value), ("number", number), ("maybe", maybe), ("items", items));
DEFINE_CONFIG(GainConfig, ("gain", gain));
DEFINE_CONFIG(SourceConfig, ("source", source));

DEFINE_CONFIG(LowPassConfig, ("cutoff", cutoff), ("order", order));
DEFINE_CONFIG(HighPassConfig, ("cutoff", cutoff));
DEFINE_CONFIG_RULES(HighPassConfig, ("cutoff", config.cutoff > 0.0, "must be positive"));
DEFINE_CONFIG_VARIANT(FilterVariant, "type", ("lowpass", LowPassConfig), ("highpass", HighPassConfig));

DEFINE_CONFIG(StageConfig, ("filter", filter), ("name", name));

// ========== Untagged variants: alternative selected by the kind of the value ==========

TEST(VariantTest, selects_alternative_by_value_kind)
{
    PropTree numbers;
    numbers.add_integer(numbers.root().id(), "value", 5);
    PropTree strings;
    strings.add_string(strings.root().id(), "value", "five");
    Diagnostics diagnostics;

    auto from_number = decode<ValueConfig>(numbers.root(), diagnostics);
    auto from_string = decode<ValueConfig>(strings.root(), diagnostics);

    ASSERT_TRUE(from_number);
    ASSERT_TRUE(from_string);
    ASSERT_TRUE(holds_alternative<int32_t>(from_number->value));
    EXPECT_EQ(get<int32_t>(from_number->value), 5);
    ASSERT_TRUE(holds_alternative<Name>(from_string->value));
    EXPECT_EQ(str(get<Name>(from_string->value)), "five");
}

TEST(VariantTest, first_accepting_alternative_wins)
{
    PropTree tree;
    tree.add_integer(tree.root().id(), "number", 3);
    Diagnostics diagnostics;

    auto config = decode<ValueConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    ASSERT_TRUE(holds_alternative<double>(config->number));
    EXPECT_DOUBLE_EQ(get<double>(config->number), 3.0);
}

TEST(VariantTest, unmatched_kind_lists_accepted_kinds)
{
    PropTree tree;
    tree.set_source_name("values.json");
    tree.add_bool(tree.root().id(), "value", true, {2, 12});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ValueConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 12, "values.json"}));
    EXPECT_EQ(str(diagnostics[0].path), "value");
    EXPECT_EQ(str(diagnostics[0].message), "expected integer or string, got bool");
}

TEST(VariantTest, optional_variant_accepts_null)
{
    PropTree tree;
    tree.add_null(tree.root().id(), "maybe");
    Diagnostics diagnostics;

    auto config = decode<ValueConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_FALSE(config->maybe.has_value());

    PropTree named;
    named.add_string(named.root().id(), "maybe", "x");
    auto named_config = decode<ValueConfig>(named.root(), diagnostics);
    ASSERT_TRUE(named_config);
    ASSERT_TRUE(named_config->maybe.has_value());
    EXPECT_TRUE(holds_alternative<Name>(named_config->maybe.value()));
}

TEST(VariantTest, array_of_variants_reports_element_path)
{
    PropTree tree;
    auto items = tree.add_array(tree.root().id(), "items", {2, 12});
    tree.add_integer(*items, "", 1, {2, 13});
    tree.add_string(*items, "", "two", {2, 16});
    tree.add_float(*items, "", 3.5, {2, 23});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ValueConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].path), "items[2]");
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 23}));
    EXPECT_EQ(str(diagnostics[0].message), "expected integer or string, got float");
}

TEST(VariantTest, struct_alternative_is_decoded_from_object)
{
    PropTree tree;
    auto source = tree.add_object(tree.root().id(), "source", {2, 13});
    tree.add_float(*source, "gain", 0.5, {2, 23});
    Diagnostics diagnostics;

    auto config = decode<SourceConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    ASSERT_TRUE(holds_alternative<GainConfig>(config->source));
    EXPECT_DOUBLE_EQ(get<GainConfig>(config->source).gain, 0.5);
}

TEST(VariantTest, warnings_of_selected_alternative_are_reported)
{
    PropTree tree;
    auto source = tree.add_object(tree.root().id(), "source", {2, 13});
    tree.add_float(*source, "gian", 0.5, {2, 15});
    Diagnostics diagnostics;

    EXPECT_TRUE(decode<SourceConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].severity, Severity::Warning);
    EXPECT_EQ(str(diagnostics[0].path), "source.gian");
    EXPECT_EQ(str(diagnostics[0].message), "unknown field 'gian'");
}

// ========== Tagged variants (DEFINE_CONFIG_VARIANT) ==========

namespace {

/*
 * 1  {
 * 2    "filter": { "type": "<type>", "cutoff": <cutoff> },
 * 3    "name": "stage"
 * 4  }
 */
PropTree makeStageTree(char const* type, double cutoff)
{
    PropTree tree{SourceLocation{1, 1}};
    tree.set_source_name("stage.json");
    NodeId root = tree.root().id();
    auto filter = tree.add_object(root, "filter", {2, 13});
    tree.add_string(*filter, "type", type, {2, 23});
    tree.add_float(*filter, "cutoff", cutoff, {2, 43});
    tree.add_string(root, "name", "stage", {3, 11});
    return tree;
}

} // namespace

TEST(VariantTest, tag_selects_struct_alternative)
{
    PropTree lowpass = makeStageTree("lowpass", 50.0);
    PropTree highpass = makeStageTree("highpass", 2000.0);
    Diagnostics diagnostics;

    auto low = decode<StageConfig>(lowpass.root(), diagnostics);
    auto high = decode<StageConfig>(highpass.root(), diagnostics);

    EXPECT_TRUE(diagnostics.empty());
    ASSERT_TRUE(low);
    ASSERT_TRUE(holds_alternative<LowPassConfig>(low->filter));
    EXPECT_DOUBLE_EQ(get<LowPassConfig>(low->filter).cutoff, 50.0);
    EXPECT_EQ(get<LowPassConfig>(low->filter).order, 2);

    ASSERT_TRUE(high);
    ASSERT_TRUE(holds_alternative<HighPassConfig>(high->filter));
    EXPECT_DOUBLE_EQ(get<HighPassConfig>(high->filter).cutoff, 2000.0);
}

TEST(VariantTest, unknown_tag_is_reported_at_tag_value)
{
    PropTree tree = makeStageTree("bandpass", 50.0);
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 23, "stage.json"}));
    EXPECT_EQ(str(diagnostics[0].path), "filter.type");
    EXPECT_EQ(str(diagnostics[0].message), "unknown type 'bandpass' (expected lowpass, highpass)");
}

TEST(VariantTest, rules_of_selected_alternative_are_applied)
{
    PropTree tree = makeStageTree("highpass", -1.0);
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 43, "stage.json"}));
    EXPECT_EQ(str(diagnostics[0].path), "filter.cutoff");
    EXPECT_EQ(str(diagnostics[0].message), "must be positive");
}

TEST(VariantTest, fields_of_other_alternative_are_unknown)
{
    PropTree tree = makeStageTree("highpass", 2000.0);
    PropNode filter = tree.root().child("filter");
    tree.add_integer(filter.id(), "order", 4, {2, 55});
    Diagnostics diagnostics;

    EXPECT_TRUE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].severity, Severity::Warning);
    EXPECT_EQ(str(diagnostics[0].path), "filter.order");
    EXPECT_EQ(str(diagnostics[0].message), "unknown field 'order'");
}

TEST(VariantTest, missing_tag_is_reported_at_object)
{
    PropTree tree;
    auto filter = tree.add_object(tree.root().id(), "filter", {2, 13});
    tree.add_float(*filter, "cutoff", 1.0, {2, 25});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 13}));
    EXPECT_EQ(str(diagnostics[0].message), "missing required field 'type'");
}

TEST(VariantTest, tag_must_be_a_string)
{
    PropTree tree;
    auto filter = tree.add_object(tree.root().id(), "filter", {2, 13});
    tree.add_integer(*filter, "type", 1, {2, 23});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].path), "filter.type");
    EXPECT_EQ(str(diagnostics[0].message), "expected string, got integer");
}

TEST(VariantTest, tagged_variant_must_be_an_object)
{
    PropTree tree;
    tree.add_string(tree.root().id(), "filter", "lowpass", {2, 13});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<StageConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "expected object, got string");
}
