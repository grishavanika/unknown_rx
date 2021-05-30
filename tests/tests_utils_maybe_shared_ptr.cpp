#include "utils_maybe_shared_ptr.h"
#include <gtest/gtest.h>
using namespace testing;
using namespace xrx::detail;

TEST(MaybeShared, Construct_FromShared)
{
    auto shared = std::make_shared<int>(42);
    MaybeSharedPtr<int> p1 = MaybeSharedPtr<int>::from_shared(std::move(shared));
    ASSERT_TRUE(p1);
    ASSERT_EQ(42, *p1);
    MaybeWeakPtr<int> weak1(p1);
    ASSERT_TRUE(weak1.lock());
    ASSERT_EQ(42, *weak1.lock());
}

TEST(MaybeShared, Construct_FromRawReference)
{
    int ref = 42;
    MaybeSharedPtr<int> p2 = MaybeSharedPtr<int>::from_ref(ref);
    ASSERT_TRUE(p2);
    ASSERT_EQ(42, *p2);
    MaybeWeakPtr<int> weak2(p2);
    ASSERT_TRUE(weak2.lock());
    ASSERT_EQ(42, *weak2.lock());
}
