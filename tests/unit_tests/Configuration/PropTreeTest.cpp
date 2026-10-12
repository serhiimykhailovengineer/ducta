#include <gtest/gtest.h>

#include <string>

#include "ducta/Configuration/PropTree.hpp"

using namespace ducta;
using namespace ducta::Configuration;

namespace {

template <class StringType>
StringView str(StringType const& s)
{
    return StringView{s.data(), s.size()};
}

} // namespace

TEST(PropTreeTest, starts_with_empty_root_object)
{
    PropTree tree;

    PropNode root = tree.root();
    EXPECT_TRUE(root.valid());
    EXPECT_TRUE(root.is_object());
    EXPECT_EQ(root.size(), 0u);
    EXPECT_EQ(tree.size(), 1u);
    EXPECT_FALSE(root.parent().valid());
    EXPECT_EQ(str(root.path()), "");
}

TEST(PropTreeTest, stores_values_with_locations)
{
    PropTree tree{SourceLocation{1, 1}};
    NodeId root = tree.root().id();

    ASSERT_TRUE(tree.add_bool(root, "enabled", true, {2, 5}));
    ASSERT_TRUE(tree.add_integer(root, "count", -42, {3, 5}));
    ASSERT_TRUE(tree.add_float(root, "gain", 0.5, {4, 5}));
    ASSERT_TRUE(tree.add_string(root, "name", "adder", {5, 5}));
    ASSERT_TRUE(tree.add_null(root, "limit", {6, 5}));

    PropNode node = tree.root();
    EXPECT_EQ(node.location(), (SourceLocation{1, 1}));
    EXPECT_EQ(node.size(), 5u);

    EXPECT_EQ(node.child("enabled").kind(), NodeKind::Bool);
    EXPECT_TRUE(node.child("enabled").as_bool());
    EXPECT_EQ(node.child("enabled").location(), (SourceLocation{2, 5}));

    EXPECT_EQ(node.child("count").as_integer(), -42);
    EXPECT_EQ(node.child("count").location(), (SourceLocation{3, 5}));

    EXPECT_DOUBLE_EQ(node.child("gain").as_float(), 0.5);
    EXPECT_EQ(str(node.child("name").as_string()), "adder");
    EXPECT_TRUE(node.child("limit").is_null());
    EXPECT_EQ(node.child("limit").location(), (SourceLocation{6, 5}));
}

TEST(PropTreeTest, keeps_insertion_order)
{
    PropTree tree;
    NodeId root = tree.root().id();
    ASSERT_TRUE(tree.add_integer(root, "c", 1));
    ASSERT_TRUE(tree.add_integer(root, "a", 2));
    ASSERT_TRUE(tree.add_integer(root, "b", 3));

    std::string keys;
    for (PropNode child : tree.root())
    {
        StringView key = child.key();
        keys.append(key.data(), key.size());
    }
    EXPECT_EQ(keys, "cab");
    EXPECT_EQ(str(tree.root().at(1).key()), "a");
    EXPECT_FALSE(tree.root().at(3).valid());
}

TEST(PropTreeTest, nested_objects_and_arrays_have_paths)
{
    PropTree tree;
    auto filter = tree.add_object(tree.root().id(), "filter", {2, 3});
    ASSERT_TRUE(filter);
    auto taps = tree.add_array(*filter, "taps", {3, 5});
    ASSERT_TRUE(taps);
    ASSERT_TRUE(tree.add_integer(*taps, "", 1, {3, 14}));
    ASSERT_TRUE(tree.add_integer(*taps, "", 2, {3, 17}));
    auto item = tree.add_object(*taps, "", {4, 7});
    ASSERT_TRUE(item);
    ASSERT_TRUE(tree.add_string(*item, "name", "x", {4, 9}));

    PropNode taps_node = tree.root().child("filter").child("taps");
    ASSERT_TRUE(taps_node.is_array());
    EXPECT_EQ(taps_node.size(), 3u);
    EXPECT_EQ(taps_node.at(1).as_integer(), 2);
    EXPECT_EQ(taps_node.at(1).location(), (SourceLocation{3, 17}));
    EXPECT_TRUE(taps_node.at(1).key().empty());

    EXPECT_EQ(str(taps_node.path()), "filter.taps");
    EXPECT_EQ(str(taps_node.at(1).path()), "filter.taps[1]");
    EXPECT_EQ(str(taps_node.at(2).child("name").path()), "filter.taps[2].name");
    EXPECT_EQ(taps_node.at(2).child("name").parent().parent().id(), taps_node.id());
}

TEST(PropTreeTest, rejects_invalid_structure)
{
    PropTree tree;
    NodeId root = tree.root().id();
    auto value = tree.add_integer(root, "value", 1);
    auto list = tree.add_array(root, "list");
    ASSERT_TRUE(value);
    ASSERT_TRUE(list);

    EXPECT_FALSE(tree.add_integer(root, "value", 2, {7, 3}));
    EXPECT_FALSE(tree.add_integer(root, "", 2));
    EXPECT_FALSE(tree.add_integer(*list, "key", 2));
    EXPECT_FALSE(tree.add_integer(*value, "child", 2));
    EXPECT_FALSE(tree.add_integer(1000, "orphan", 2));

    EXPECT_EQ(tree.size(), 3u);
    EXPECT_EQ(tree.root().child("value").as_integer(), 1);
}

TEST(PropTreeTest, duplicate_key_error_mentions_location)
{
    PropTree tree;
    NodeId root = tree.root().id();
    ASSERT_TRUE(tree.add_integer(root, "gain", 1, {2, 3}));

    auto result = tree.add_integer(root, "gain", 2, {5, 3});
    ASSERT_FALSE(result);
    EXPECT_EQ(str(result.error().message), "duplicate key 'gain' at 5:3");
}

TEST(PropTreeTest, invalid_handle_is_safe_to_query)
{
    PropTree tree;
    PropNode missing = tree.root().child("missing");

    EXPECT_FALSE(missing.valid());
    EXPECT_FALSE(missing.is_null());
    EXPECT_FALSE(missing.is_object());
    EXPECT_EQ(missing.size(), 0u);
    EXPECT_FALSE(missing.child("x").valid());
    EXPECT_FALSE(missing.location().valid());
    EXPECT_TRUE(missing.key().empty());
    EXPECT_EQ(str(missing.path()), "");
    EXPECT_TRUE(missing.begin() == missing.end());
}

TEST(PropTreeTest, locations_carry_source_name)
{
    PropTree tree;
    tree.set_source_name("configs/pipeline.json");
    ASSERT_TRUE(tree.add_integer(tree.root().id(), "value", 1, {2, 5}));

    SourceLocation location = tree.root().child("value").location();

    EXPECT_EQ(str(tree.source_name()), "configs/pipeline.json");
    EXPECT_STREQ(location.file_name(), "configs/pipeline.json");
    EXPECT_STREQ(location.function_name(), "");
    EXPECT_EQ(location.line(), 2u);
    EXPECT_EQ(location.column(), 5u);
    EXPECT_EQ(location, (SourceLocation{2, 5, "configs/pipeline.json"}));
}

TEST(PropTreeTest, stream_source_has_empty_file_name)
{
    PropTree tree;
    ASSERT_TRUE(tree.add_integer(tree.root().id(), "value", 1, {2, 5}));

    EXPECT_STREQ(tree.root().child("value").location().file_name(), "");
}

TEST(PropTreeTest, only_line_and_column_of_added_location_are_stored)
{
    PropTree tree;
    tree.set_source_name("a.yaml");
    ASSERT_TRUE(tree.add_integer(tree.root().id(), "value", 1, {3, 7, "other.yaml", "loader"}));

    EXPECT_EQ(tree.root().child("value").location(), (SourceLocation{3, 7, "a.yaml"}));
}

TEST(PropTreeTest, clear_resets_to_empty_root)
{
    PropTree tree;
    ASSERT_TRUE(tree.add_integer(tree.root().id(), "value", 1));

    tree.set_source_name("a.json");
    tree.clear(SourceLocation{1, 1});

    EXPECT_EQ(tree.size(), 1u);
    EXPECT_TRUE(tree.source_name().empty());
    EXPECT_EQ(tree.root().size(), 0u);
    EXPECT_EQ(tree.root().location(), (SourceLocation{1, 1}));
}
