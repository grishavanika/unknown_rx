#include "utils_containers.h"
#include <vector>
#include <algorithm>
#include <gtest/gtest.h>

using namespace xrx::detail;

template<typename V>
static bool equal_to_(V& v, std::initializer_list<int> to)
{
    std::vector<int> data;
    v.for_each([&](int e) { data.push_back(e); return true; });
    if (data.size() == to.size())
    {
        return std::equal(data.cbegin(), data.cend(), to.begin());
    }
    return false;
}

TEST(SmallVector, DefaultConstructible_Empty)
{
    SmallVector<int, 1> v;
    ASSERT_EQ(std::size_t(0), v.size());
    ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v, {}));
}

TEST(SmallVector, CopyConstructible_Empty)
{
    SmallVector<int, 1> v;
    SmallVector<int, 1> v_copy = v;
    ASSERT_EQ(std::size_t(0), v_copy.size());
    ASSERT_EQ(std::size_t(0), v_copy.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v_copy, {}));
}

TEST(SmallVector, MoveConstructible_Empty)
{
    SmallVector<int, 1> v;
    SmallVector<int, 1> v_copy = std::move(v);
    {
        ASSERT_EQ(std::size_t(0), v_copy.size());
        ASSERT_EQ(std::size_t(0), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {}));
    }
    {
        ASSERT_EQ(std::size_t(0), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {}));
    }
}

TEST(SmallVector, ReserveCapacity)
{
    SmallVector<int, 1> v(10);
    ASSERT_EQ(std::size_t(0), v.size());
    ASSERT_EQ(std::size_t(9), v.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v, {}));
}

TEST(SmallVector, ReserveCapacity_CopyConstructible_Empty)
{
    SmallVector<int, 1> v(10);
    SmallVector<int, 1> v_copy = v;
    ASSERT_EQ(std::size_t(0), v_copy.size());
    ASSERT_EQ(std::size_t(9), v_copy.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v_copy, {}));
}

TEST(SmallVector, ReserveCapacity_MoveConstructible_Empty)
{
    SmallVector<int, 1> v(10);
    SmallVector<int, 1> v_copy = std::move(v);
    {
        ASSERT_EQ(std::size_t(0), v_copy.size());
        ASSERT_EQ(std::size_t(9), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {}));
    }
    {
        ASSERT_EQ(std::size_t(0), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {}));
    }
}

TEST(SmallVector, PushBack_Static)
{
    SmallVector<int, 1> v;
    v.push_back(42);
    ASSERT_EQ(std::size_t(1), v.size());
    ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v, {42}));
}

TEST(SmallVector, PushBack_Static_Copy)
{
    SmallVector<int, 1> v;
    v.push_back(42);
    SmallVector<int, 1> v_copy = v;
    {
        ASSERT_EQ(std::size_t(1), v_copy.size());
        ASSERT_EQ(std::size_t(0), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42}));
    }
    {
        ASSERT_EQ(std::size_t(1), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42}));
    }
}

TEST(SmallVector, PushBack_Static_Move)
{
    SmallVector<int, 1> v;
    v.push_back(42);
    SmallVector<int, 1> v_copy = std::move(v);
    {
        ASSERT_EQ(std::size_t(1), v_copy.size());
        ASSERT_EQ(std::size_t(0), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42}));
    }
    {
        ASSERT_EQ(std::size_t(0), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {}));
    }
}

TEST(SmallVector, PushBack_Dynamic)
{
    SmallVector<int, 1> v(2);
    v.push_back(42);
    v.push_back(43);
    ASSERT_EQ(std::size_t(2), v.size());
    ASSERT_EQ(std::size_t(1), v.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v, {42, 43}));
}

TEST(SmallVector, PushBack_Dynamic_Copy)
{
    SmallVector<int, 1> v(2);
    v.push_back(42);
    v.push_back(43);
    SmallVector<int, 1> v_copy = v;
    {
        ASSERT_EQ(std::size_t(2), v_copy.size());
        ASSERT_EQ(std::size_t(1), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42, 43}));
    }
    {
        ASSERT_EQ(std::size_t(2), v.size());
        ASSERT_EQ(std::size_t(1), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {42, 43}));
    }
}

TEST(SmallVector, PushBack_Dynamic_Move)
{
    SmallVector<int, 1> v(2);
    v.push_back(42);
    v.push_back(43);
    SmallVector<int, 1> v_copy = std::move(v);
    {
        ASSERT_EQ(std::size_t(2), v_copy.size());
        ASSERT_EQ(std::size_t(1), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42, 43}));
    }
    {
        ASSERT_EQ(std::size_t(0), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {}));
    }
}

TEST(SmallVector, PushBack_Dynamic_WithCapacityLeft)
{
    SmallVector<int, 1> v(10);
    v.push_back(42);
    v.push_back(43);
    ASSERT_EQ(std::size_t(2), v.size());
    ASSERT_EQ(std::size_t(9), v.dynamic_capacity());
    ASSERT_TRUE(equal_to_(v, {42, 43}));
}

TEST(SmallVector, PushBack_Dynamic_Copy_WithCapacityLeft)
{
    SmallVector<int, 1> v(10);
    v.push_back(42);
    v.push_back(43);
    SmallVector<int, 1> v_copy = v;
    {
        ASSERT_EQ(std::size_t(2), v_copy.size());
        ASSERT_EQ(std::size_t(9), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42, 43}));
    }
    {
        ASSERT_EQ(std::size_t(2), v.size());
        ASSERT_EQ(std::size_t(9), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {42, 43}));
    }
}

TEST(SmallVector, PushBack_Dynamic_Move_WithCapacityLeft)
{
    SmallVector<int, 1> v(10);
    v.push_back(42);
    v.push_back(43);
    SmallVector<int, 1> v_copy = std::move(v);
    {
        ASSERT_EQ(std::size_t(2), v_copy.size());
        ASSERT_EQ(std::size_t(9), v_copy.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v_copy, {42, 43}));
    }
    {
        ASSERT_EQ(std::size_t(0), v.size());
        ASSERT_EQ(std::size_t(0), v.dynamic_capacity());
        ASSERT_TRUE(equal_to_(v, {}));
    }
}


