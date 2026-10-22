#include <gtest/gtest.h>

#include "ducta/IO/DeclareEvent.hpp"

/* Declared at global scope, as an application does for a type of another library */
struct GlobalEvent
{
    int value;
};

namespace app {
struct NamespacedEvent
{
    int value;
};
struct UnnamedEvent
{
    int value;
};
} // namespace app

DUCTA_DECLARE_EVENT_NAME(GlobalEvent, "GlobalEvent");
DUCTA_DECLARE_EVENT_NAME(app::NamespacedEvent, "app.NamespacedEvent");

namespace {

TEST(DeclareEventTest, DeclaredNameIsUsed)
{
    EXPECT_TRUE(ducta::Private::type_name<GlobalEvent>() == ducta::StringView{"GlobalEvent"});
    EXPECT_TRUE(ducta::Private::type_name<app::NamespacedEvent const&>() ==
                ducta::StringView{"app.NamespacedEvent"});
}

TEST(DeclareEventTest, UndeclaredNameIsTheTypeName)
{
    EXPECT_FALSE(ducta::Private::type_name<app::UnnamedEvent>().empty());
    EXPECT_FALSE(ducta::Private::type_name<app::UnnamedEvent>() == ducta::StringView{"GlobalEvent"});
}

TEST(DeclareEventTest, DeclaredNameGivesOneTypeId)
{
    EXPECT_EQ(DUCTA_TYPE_ID(GlobalEvent), DUCTA_TYPE_ID(GlobalEvent const));
    EXPECT_NE(DUCTA_TYPE_ID(GlobalEvent), DUCTA_TYPE_ID(app::NamespacedEvent));
}

} // namespace
