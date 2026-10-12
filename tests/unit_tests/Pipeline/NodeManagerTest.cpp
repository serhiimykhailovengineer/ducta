#include <gtest/gtest.h>

#include <string>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/Pipeline/NodeManager.hpp"

#include "TestNodes.hpp"

using namespace ducta;
using namespace ducta::Configuration;
using namespace ducta::Pipeline;

namespace {

PropTree loadTree(char const* text)
{
    PropTree tree;
    Diagnostics diagnostics;
    EXPECT_TRUE(loadJson(text, tree, diagnostics)) << text;
    return tree;
}

template <class StringType>
std::string str(StringType const& s)
{
    return std::string{s.data(), s.size()};
}

void add(NodeManagerConfig& config, char const* name, char const* type, PropNode node = {})
{
    config.nodes.insert(
        ::ducta::pair<NodeName const, NodeSpec>(NodeName{name}, NodeSpec{NodeName{type}, node}));
}

/* Large enough for every test node */
using SmallStatic = StaticNodeStorage<512, 8>;

} // namespace

// Runs every test with the static storage and, on the standard backend, with the heap storage
template <class Storage>
class NodeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestNodes::CountingNode::alive = 0;
        TestNodes::GainNode::last_gain = 0.0;
    }
};

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using Storages = ::testing::Types<SmallStatic>;
#else
using Storages = ::testing::Types<SmallStatic, HeapNodeStorage>;
#endif
TYPED_TEST_SUITE(NodeManagerTest, Storages);

TYPED_TEST(NodeManagerTest, configure_from_prop_tree)
{
    PropTree tree =
        loadTree(R"({ "nodes": { "Gain": { "type": "Gain", "config": { "gain": 2.5, "label": "amp" } },
                                 "Plain": { "type": "Plain" } } })");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(tree, diagnostics));

    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(manager.size(), 2u);
    EXPECT_DOUBLE_EQ(TestNodes::GainNode::last_gain, 2.5);

    auto gain = manager.nodes().get_node("Gain");
    ASSERT_TRUE(gain.has_value());
    gain->node.init();
    gain->node.release();
    EXPECT_TRUE(manager.nodes().get_node("Plain").has_value());
}

TYPED_TEST(NodeManagerTest, configure_from_config)
{
    PropTree tree = loadTree(R"({ "gain": 4.0 })");
    NodeManagerConfig config;
    add(config, "Gain", "Gain", tree.root());
    add(config, "Counting", "Counting");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(config, diagnostics));

    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(manager.size(), 2u);
    EXPECT_DOUBLE_EQ(TestNodes::GainNode::last_gain, 4.0);
    EXPECT_EQ(TestNodes::CountingNode::alive, 1);
}

TYPED_TEST(NodeManagerTest, several_instances_of_one_type)
{
    PropTree tree = loadTree(R"({ "nodes": {
        "left":   { "type": "Counting" },
        "right":  { "type": "Counting" },
        "loud":   { "type": "Gain", "config": { "gain": 4.0 } },
        "quiet":  { "type": "Gain", "config": { "gain": 0.5 } },
        "quiet2": { "type": "Gain", "config": { "gain": 0.5 } }
    } })");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(tree, diagnostics));
    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(manager.size(), 5u);
    EXPECT_EQ(TestNodes::CountingNode::alive, 2);
    for (char const* name : {"left", "right", "loud", "quiet", "quiet2"})
    {
        EXPECT_TRUE(manager.nodes().get_node(name).has_value()) << name;
    }
    EXPECT_FALSE(manager.nodes().get_node("Gain").has_value());
}

TYPED_TEST(NodeManagerTest, type_is_required)
{
    PropTree tree = loadTree("{ \"nodes\": {\n  \"amp\": { \"config\": { \"gain\": 2.0 } } } }");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    EXPECT_FALSE(manager.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "missing required field 'type'");
    EXPECT_EQ(str(diagnostics[0].path), "nodes.amp");
    EXPECT_EQ(diagnostics[0].location.line(), 2u);
}

TYPED_TEST(NodeManagerTest, missing_node_config_uses_defaults)
{
    NodeManagerConfig config;
    add(config, "Gain", "Gain");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(config, diagnostics));
    EXPECT_DOUBLE_EQ(TestNodes::GainNode::last_gain, 1.0);
}

TYPED_TEST(NodeManagerTest, unknown_parameters_are_warnings)
{
    PropTree tree = loadTree(
        "{ \"nodes\": {\n  \"Gain\": { \"type\": \"Gain\", \"config\": { \"gain\": 2.0, \"extra\": "
        "1 } },\n"
        "  \"Plain\": { \"type\": \"Plain\", \"config\": { \"value\": 3 } } } }");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(tree, diagnostics));

    EXPECT_EQ(manager.size(), 2u);
    EXPECT_FALSE(diagnostics.has_errors());
    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(diagnostics[0].severity, Severity::Warning);
    EXPECT_EQ(str(diagnostics[0].path), "nodes.Gain.config.extra");
    EXPECT_EQ(diagnostics[0].location.line(), 2u);
    EXPECT_EQ(diagnostics[1].severity, Severity::Warning);
    EXPECT_EQ(str(diagnostics[1].path), "nodes.Plain.config");
    EXPECT_EQ(
        str(diagnostics[1].message), "node type takes no configuration, parameters are ignored");
}

TYPED_TEST(NodeManagerTest, unknown_type_fails_and_keeps_no_node)
{
    PropTree tree = loadTree(
        "{ \"nodes\": {\n  \"Counting\": { \"type\": \"Counting\" },\n"
        "  \"Nope\": { \"type\": \"Nope\", \"config\": {} } } }");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    EXPECT_FALSE(manager.configure(tree, diagnostics));

    EXPECT_EQ(manager.size(), 0u);
    EXPECT_FALSE(manager.nodes().get_node("Counting").has_value());
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "unknown node type 'Nope' of node 'Nope'");
    EXPECT_EQ(diagnostics[0].location.line(), 3u);
}

TYPED_TEST(NodeManagerTest, config_errors_fail_and_keep_no_node)
{
    PropTree tree = loadTree(
        "{ \"nodes\": {\n  \"Counting\": { \"type\": \"Counting\" },\n"
        "  \"Gain\": { \"type\": \"Gain\", \"config\": { \"gain\": 0 } } } }");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    EXPECT_FALSE(manager.configure(tree, diagnostics));

    EXPECT_EQ(manager.size(), 0u);
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].path), "nodes.Gain.config.gain");
    EXPECT_EQ(str(diagnostics[0].message), "must be positive");
}

TYPED_TEST(NodeManagerTest, invalid_manager_config_is_reported)
{
    PropTree tree = loadTree(R"({ "nodes": [1, 2] })");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    EXPECT_FALSE(manager.configure(tree, diagnostics));
    EXPECT_TRUE(diagnostics.has_errors());
    EXPECT_EQ(manager.size(), 0u);
}

TYPED_TEST(NodeManagerTest, reconfigure_replaces_nodes)
{
    NodeManagerConfig first;
    add(first, "Counting", "Counting");
    NodeManagerConfig second;
    add(second, "Plain", "Plain");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(first, diagnostics));
    EXPECT_EQ(TestNodes::CountingNode::alive, 1);

    ASSERT_TRUE(manager.configure(second, diagnostics));
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    EXPECT_EQ(manager.size(), 1u);
    EXPECT_FALSE(manager.nodes().get_node("Counting").has_value());
    EXPECT_TRUE(manager.nodes().get_node("Plain").has_value());

    manager.clear();
    EXPECT_EQ(manager.size(), 0u);
    EXPECT_FALSE(manager.nodes().get_node("Plain").has_value());
}

TYPED_TEST(NodeManagerTest, destructor_destroys_nodes)
{
    NodeManagerConfig config;
    add(config, "Counting", "Counting");
    Diagnostics diagnostics;
    {
        NodeManager<TypeParam> manager;
        ASSERT_TRUE(manager.configure(config, diagnostics));
        EXPECT_EQ(TestNodes::CountingNode::alive, 1);
    }
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
}

TYPED_TEST(NodeManagerTest, uses_given_factory)
{
    NodeFactory factory;
    factory.register_type<TestNodes::PlainNode>("Only");
    NodeManagerConfig config;
    add(config, "Only", "Only");
    add(config, "Gain", "Gain");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager{factory};

    EXPECT_FALSE(manager.configure(config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "unknown node type 'Gain' of node 'Gain'");
}

#if defined(__cpp_exceptions)
TYPED_TEST(NodeManagerTest, throwing_constructor_keeps_no_node)
{
    PropTree tree = loadTree(R"({ "nodes": {
        "a": { "type": "Counting" },
        "b": { "type": "Throwing", "config": { "in_constructor": true } }
    } })");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    EXPECT_THROW(manager.configure(tree, diagnostics), std::runtime_error);
    EXPECT_EQ(manager.size(), 0u);
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    EXPECT_FALSE(manager.nodes().get_node("a").has_value());
}
#endif

TYPED_TEST(NodeManagerTest, over_aligned_node)
{
    NodeManagerConfig config;
    add(config, "Plain", "Plain");
    add(config, "Aligned", "Aligned");
    Diagnostics diagnostics;
    NodeManager<TypeParam> manager;

    ASSERT_TRUE(manager.configure(config, diagnostics));
    EXPECT_EQ(manager.size(), 2u);
}

TEST(StaticNodeStorageTest, full_storage_is_reported)
{
    NodeManagerConfig config;
    add(config, "Counting", "Counting");
    add(config, "Plain", "Plain");
    Diagnostics diagnostics;
    TestNodes::CountingNode::alive = 0;

    NodeManager<StaticNodeStorage<64, 1>> manager;
    EXPECT_FALSE(manager.configure(config, diagnostics));

    EXPECT_EQ(manager.size(), 0u);
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "not enough node storage");
}

TEST(StaticNodeStorageTest, too_small_buffer_is_reported)
{
    NodeManagerConfig config;
    add(config, "Gain", "Gain");
    Diagnostics diagnostics;

    NodeManager<StaticNodeStorage<sizeof(TestNodes::GainNode) - 1, 4>> manager;
    EXPECT_FALSE(manager.configure(config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "not enough node storage");
}

TEST(StaticNodeStorageTest, places_nodes_aligned_in_buffer)
{
    StaticNodeStorage<256, 4> storage;

    void* first = storage.allocate(3, 1);
    ASSERT_NE(first, nullptr);
    storage.adopt(first, 1, &Pipeline::Private::destroy_node<unsigned char>);

    void* second = storage.allocate(8, 64);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(second) % 64, 0u);
    storage.adopt(second, 64, &Pipeline::Private::destroy_node<unsigned char>);

    EXPECT_EQ(storage.size(), 2u);
    storage.clear();
    EXPECT_EQ(storage.size(), 0u);
    EXPECT_EQ(storage.used(), 0u);
}

#if defined(__cpp_exceptions)
namespace {

/* Gives a storage policy the NodeStorage interface, as NodeManager does */
template <class Storage>
class StorageAdapter final : public NodeStorage
{
public:
    explicit StorageAdapter(Storage& storage)
        : m_storage{storage}
    {
    }

private:
    void* allocate(size_t size, size_t align) noexcept override
    {
        return m_storage.allocate(size, align);
    }

    void deallocate(void* memory, size_t align) noexcept override
    {
        m_storage.deallocate(memory, align);
    }

    void adopt(NodeRef, void* object, size_t align, NodeDestroyFn destroy) override
    {
        m_storage.adopt(object, align, destroy);
    }

    Storage& m_storage;
};

} // namespace

TEST(StaticNodeStorageTest, memory_of_a_throwing_constructor_is_given_back)
{
    StaticNodeStorage<512, 4> storage;
    StorageAdapter<StaticNodeStorage<512, 4>> adapter{storage};

    ASSERT_NE(adapter.construct<TestNodes::CountingNode>(), nullptr);
    size_t const used = storage.used();

    TestNodes::ThrowingConfig const throwing{true, false};
    EXPECT_THROW(adapter.construct<TestNodes::ThrowingNode>(throwing), std::runtime_error);
    EXPECT_EQ(storage.used(), used);
    EXPECT_EQ(storage.size(), 1u);

    storage.clear();
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
}

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
// A leak would be reported by LeakSanitizer
TEST(HeapNodeStorageTest, memory_of_a_throwing_constructor_is_freed)
{
    HeapNodeStorage storage;
    StorageAdapter<HeapNodeStorage> adapter{storage};

    TestNodes::ThrowingConfig const throwing{true, false};
    EXPECT_THROW(adapter.construct<TestNodes::ThrowingNode>(throwing), std::runtime_error);
    EXPECT_EQ(storage.size(), 0u);
}
#endif
#endif
