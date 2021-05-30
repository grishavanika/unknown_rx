#include "utils_containers.h"
#include "debug/tracking_allocator.h"
#include <vector>
#include <algorithm>
#include <gtest/gtest.h>

using namespace xrx::detail;
using namespace xrx::debug;

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

TEST(HandleVector, Iterator)
{
    HandleVector<int> vs;
    vs.push_back(42);
    vs.push_back(43);
    std::vector<int> copy;
    for (int& v : vs)
    {
        copy.push_back(v);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(43, copy[1]);
}

TEST(HandleVector, Iterator_SkipsFirstInvalid)
{
    HandleVector<int> vs;
    auto handle1 = vs.push_back(42);
    auto handle2 = vs.push_back(43);
    vs.push_back(44);
    vs.push_back(45);

    ASSERT_EQ(4, int(vs.size()));
    ASSERT_TRUE(vs.erase(handle1));
    ASSERT_TRUE(vs.erase(handle2));
    ASSERT_EQ(2, int(vs.size()));

    std::vector<int> copy;
    for (int& v : vs)
    {
        copy.push_back(v);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(44, copy[0]);
    ASSERT_EQ(45, copy[1]);
}

TEST(HandleVector, Iterator_SkipsMiddleInvalid)
{
    HandleVector<int> vs;
    vs.push_back(42);
    auto handle1 = vs.push_back(43);
    auto handle2 = vs.push_back(44);
    vs.push_back(45);

    ASSERT_EQ(4, int(vs.size()));
    ASSERT_TRUE(vs.erase(handle1));
    ASSERT_TRUE(vs.erase(handle2));
    ASSERT_EQ(2, int(vs.size()));

    std::vector<int> copy;
    for (int& v : vs)
    {
        copy.push_back(v);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(45, copy[1]);
}

TEST(HandleVector, Iterator_SkipsLastInvalid)
{
    HandleVector<int> vs;
    vs.push_back(42);
    vs.push_back(43);
    auto handle1 = vs.push_back(44);
    auto handle2 = vs.push_back(45);

    ASSERT_EQ(4, int(vs.size()));
    ASSERT_TRUE(vs.erase(handle1));
    ASSERT_TRUE(vs.erase(handle2));
    ASSERT_EQ(2, int(vs.size()));

    std::vector<int> copy;
    for (int& v : vs)
    {
        copy.push_back(v);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(43, copy[1]);
}

TEST(HandleVector, Iterator_CanEraseHandle_WhileIterating)
{
    HandleVector<int> vs;
    vs.push_back(42);
    auto handle1 = vs.push_back(44);
    vs.push_back(45);

    std::vector<int> copy;
    for (int& v : vs)
    {
        copy.push_back(v);
        // Remove 2nd element once.
        (void)vs.erase(handle1);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(45, copy[1]);
}

TEST(HandleVector, Iterator_WithHandle)
{
    HandleVector<int> vs;
    vs.push_back(42);
    vs.push_back(43);

    auto [ref, _] = *vs.iterate_with_handle().begin();
    static_assert(std::is_lvalue_reference_v<decltype(ref)>);

    std::vector<int> copy;
    for (auto [value, handle] : vs.iterate_with_handle())
    {
        ASSERT_TRUE(handle);
        copy.push_back(value);
    }
    ASSERT_EQ(2, int(copy.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(43, copy[1]);
}

TEST(HandleVector, Iterator_WithHandle_EraseAll)
{
    HandleVector<int> vs;

    vs.push_back(42);
    vs.push_back(43);

    std::vector<int> copy;
    for (auto&& [value, handle] : vs.iterate_with_handle())
    {
        copy.push_back(value);
        ASSERT_TRUE(vs.erase(handle));
    }
    ASSERT_EQ(0, int(vs.size()));
    ASSERT_EQ(42, copy[0]);
    ASSERT_EQ(43, copy[1]);
}

TEST(HandleVector, Construct_WithAllocator)
{
    using Alloc = TrackingAllocator<int>;
    TrackingAllocatorState stats;

    {
        HandleVector<int, Alloc> vs{Alloc(&stats)};
#if defined(NDEBUG) // Release-only. In Debug, MSVC allocates some memory for debug purpose.
        ASSERT_EQ(0, int(stats._allocs_count));
#endif
        (void)vs;
    }
    ASSERT_EQ(stats._allocs_count, stats._deallocs_count);
    ASSERT_EQ(stats._allocs_size, stats._deallocs_size);
}

TEST(HandleVector, Construct_WithAllocator_OnePush)
{
    using Alloc = TrackingAllocator<int>;
    TrackingAllocatorState stats;

    {
        HandleVector<int, Alloc> vs{Alloc(&stats)};
        (void)vs.push_back(1);
        ASSERT_EQ(1, int(stats._constructs_count));
#if defined(NDEBUG) // Release-only. In Debug, MSVC allocates some memory for debug purpose.
        ASSERT_EQ(1, int(stats._allocs_count));
#else
        ASSERT_GT(int(stats._allocs_count), 1);
#endif

        std::printf("Allocates count: %i.\n", int(stats._allocs_count));
        std::printf("Allocations size: %i.\n", int(stats._allocs_size));
    }
    ASSERT_EQ(stats._allocs_count, stats._deallocs_count);
    ASSERT_EQ(stats._allocs_size, stats._deallocs_size);
    ASSERT_EQ(stats._constructs_count, stats._destroys_count);
}
