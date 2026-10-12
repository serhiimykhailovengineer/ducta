#include <gtest/gtest.h>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/Core/Types/Array.hpp"
#include "ducta/Core/Types/Map.hpp"
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

struct ContainersConfig
{
    Name name;
    Vector<int32_t, 4> values;
    Map<Name, int32_t, 4> gains;
    Array<double, 3> coefficients{};
    Optional<Name> label;

    // Nested containers
    Vector<Optional<Name>, 4> aliases;
    Map<Name, Vector<int32_t, 4>, 4> groups;
    Array<Variant<int32_t, Name>, 2> pair{};
    Optional<Map<Name, Name, 4>> tags;
};

template <class MapType>
auto const& at(MapType const& map, char const* key)
{
    auto it = map.find(Name(key));
    EXPECT_TRUE(it != map.end()) << key;
    return it->second;
}

} // namespace

DEFINE_CONFIG(
    ContainersConfig,
    ("name", name),
    ("values", values),
    ("gains", gains),
    ("coefficients", coefficients),
    ("label", label),
    ("aliases", aliases),
    ("groups", groups),
    ("pair", pair),
    ("tags", tags));

TEST(ContainersTest, decodes_flat_containers)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_string(root, "name", "mixer");
    auto values = tree.add_array(root, "values");
    tree.add_integer(*values, "", 1);
    tree.add_integer(*values, "", 2);
    auto gains = tree.add_object(root, "gains");
    tree.add_integer(*gains, "left", 3);
    tree.add_integer(*gains, "right", -4);
    auto coefficients = tree.add_array(root, "coefficients");
    tree.add_float(*coefficients, "", 0.25);
    tree.add_integer(*coefficients, "", 1);
    tree.add_float(*coefficients, "", -0.5);
    tree.add_string(root, "label", "main");
    Diagnostics diagnostics;

    auto config = decode<ContainersConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config) << diagnostics[0].message;
    EXPECT_EQ(str(config->name), "mixer");

    ASSERT_EQ(config->values.size(), 2u);
    EXPECT_EQ(config->values[1], 2);

    EXPECT_EQ(config->gains.size(), 2u);
    EXPECT_EQ(at(config->gains, "left"), 3);
    EXPECT_EQ(at(config->gains, "right"), -4);

    EXPECT_DOUBLE_EQ(config->coefficients[0], 0.25);
    EXPECT_DOUBLE_EQ(config->coefficients[1], 1.0);
    EXPECT_DOUBLE_EQ(config->coefficients[2], -0.5);

    ASSERT_TRUE(config->label.has_value());
    EXPECT_EQ(str(config->label.value()), "main");
}

TEST(ContainersTest, decodes_nested_containers)
{
    PropTree tree;
    NodeId root = tree.root().id();
    auto aliases = tree.add_array(root, "aliases");
    tree.add_string(*aliases, "", "a");
    tree.add_null(*aliases, "");
    auto groups = tree.add_object(root, "groups");
    auto first = tree.add_array(*groups, "first");
    tree.add_integer(*first, "", 1);
    tree.add_integer(*first, "", 2);
    tree.add_array(*groups, "empty");
    auto pair = tree.add_array(root, "pair");
    tree.add_integer(*pair, "", 7);
    tree.add_string(*pair, "", "seven");
    auto tags = tree.add_object(root, "tags");
    tree.add_string(*tags, "owner", "dsp");
    Diagnostics diagnostics;

    auto config = decode<ContainersConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config) << diagnostics[0].message;

    ASSERT_EQ(config->aliases.size(), 2u);
    ASSERT_TRUE(config->aliases[0].has_value());
    EXPECT_EQ(str(config->aliases[0].value()), "a");
    EXPECT_FALSE(config->aliases[1].has_value());

    EXPECT_EQ(config->groups.size(), 2u);
    ASSERT_EQ(at(config->groups, "first").size(), 2u);
    EXPECT_EQ(at(config->groups, "first")[1], 2);
    EXPECT_TRUE(at(config->groups, "empty").empty());

    ASSERT_TRUE(holds_alternative<int32_t>(config->pair[0]));
    EXPECT_EQ(get<int32_t>(config->pair[0]), 7);
    ASSERT_TRUE(holds_alternative<Name>(config->pair[1]));
    EXPECT_EQ(str(get<Name>(config->pair[1])), "seven");

    ASSERT_TRUE(config->tags.has_value());
    EXPECT_EQ(str(at(config->tags.value(), "owner")), "dsp");
}

TEST(ContainersTest, missing_and_null_optionals_are_empty)
{
    PropTree tree;
    tree.add_null(tree.root().id(), "label");
    Diagnostics diagnostics;

    auto config = decode<ContainersConfig>(tree.root(), diagnostics);

    ASSERT_TRUE(config);
    EXPECT_FALSE(config->label.has_value());
    EXPECT_FALSE(config->tags.has_value());
}

TEST(ContainersTest, empty_containers_replace_previous_content)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_array(root, "values");
    tree.add_object(root, "gains");
    Diagnostics diagnostics;

    ContainersConfig config;
    config.values.push_back(1);
    config.gains.insert(::ducta::pair<Name const, int32_t>(Name("old"), 1));

    ASSERT_TRUE(decode(tree.root(), config, diagnostics));
    EXPECT_TRUE(config.values.empty());
    EXPECT_TRUE(config.gains.empty());
}

TEST(ContainersTest, wrong_container_kinds_are_reported)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_integer(root, "name", 1, {2, 3});
    tree.add_object(root, "values", {3, 3});
    tree.add_array(root, "gains", {4, 3});
    tree.add_string(root, "coefficients", "none", {5, 3});
    tree.add_bool(root, "label", true, {6, 3});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ContainersConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 5u);
    EXPECT_EQ(str(diagnostics[0].message), "expected string, got integer");
    EXPECT_EQ(str(diagnostics[1].message), "expected array, got object");
    EXPECT_EQ(str(diagnostics[2].message), "expected object, got array");
    EXPECT_EQ(str(diagnostics[3].message), "expected array, got string");
    EXPECT_EQ(str(diagnostics[4].message), "expected string, got bool");
    EXPECT_EQ(diagnostics[4].location, (SourceLocation{6, 3}));
}

TEST(ContainersTest, array_requires_exact_size)
{
    PropTree tree;
    auto coefficients = tree.add_array(tree.root().id(), "coefficients", {2, 19});
    tree.add_float(*coefficients, "", 1.0);
    tree.add_float(*coefficients, "", 2.0);
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ContainersConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].location, (SourceLocation{2, 19}));
    EXPECT_EQ(str(diagnostics[0].path), "coefficients");
    EXPECT_EQ(str(diagnostics[0].message), "expected 3 elements, got 2");
}

TEST(ContainersTest, element_errors_have_nested_paths)
{
    PropTree tree;
    NodeId root = tree.root().id();
    auto gains = tree.add_object(root, "gains");
    tree.add_string(*gains, "left", "loud", {3, 13});
    auto groups = tree.add_object(root, "groups");
    auto second = tree.add_array(*groups, "second");
    tree.add_integer(*second, "", 1);
    tree.add_float(*second, "", 2.5, {5, 17});
    auto pair = tree.add_array(root, "pair");
    tree.add_integer(*pair, "", 1);
    tree.add_bool(*pair, "", false, {6, 15});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ContainersConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 3u);
    EXPECT_EQ(str(diagnostics[0].path), "gains.left");
    EXPECT_EQ(str(diagnostics[0].message), "expected integer, got string");
    EXPECT_EQ(str(diagnostics[1].path), "groups.second[1]");
    EXPECT_EQ(diagnostics[1].location, (SourceLocation{5, 17}));
    EXPECT_EQ(str(diagnostics[2].path), "pair[1]");
    EXPECT_EQ(str(diagnostics[2].message), "expected integer or string, got bool");
}

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

TEST(ContainersTest, embedded_capacities_are_enforced)
{
    PropTree tree;
    NodeId root = tree.root().id();
    tree.add_string(root, "name", "a_name_longer_than_16", {2, 3});
    auto values = tree.add_array(root, "values", {3, 3});
    for (int i = 0; i < 5; ++i)
    {
        tree.add_integer(*values, "", i);
    }
    auto gains = tree.add_object(root, "gains", {4, 3});
    tree.add_integer(*gains, "a", 1);
    tree.add_integer(*gains, "b", 2);
    tree.add_integer(*gains, "c", 3);
    tree.add_integer(*gains, "d", 4);
    tree.add_integer(*gains, "e", 5);
    auto tags = tree.add_object(root, "tags");
    tree.add_string(*tags, "a_key_longer_than_16", "x", {6, 5});
    Diagnostics diagnostics;

    EXPECT_FALSE(decode<ContainersConfig>(tree.root(), diagnostics));

    ASSERT_EQ(diagnostics.size(), 4u);
    EXPECT_EQ(str(diagnostics[0].message), "string is too long (21 characters, max 16)");
    EXPECT_EQ(str(diagnostics[1].message), "too many elements (5, max 4)");
    EXPECT_EQ(str(diagnostics[2].message), "too many entries (5, max 4)");
    EXPECT_EQ(
        str(diagnostics[3].message), "key 'a_key_longer_than_16' is too long (max 16 characters)");
    EXPECT_EQ(diagnostics[3].location, (SourceLocation{6, 5}));
}

#endif
