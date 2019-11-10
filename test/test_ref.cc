#include <set>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ref.h"

struct TestClsMocks
{
    MOCK_METHOD(void, constructor, ());
    MOCK_METHOD(void, constructor, (float, std::string));
    MOCK_METHOD(void, constructorWithPF, ());
    MOCK_METHOD(void, destructor, (void*));

    MOCK_METHOD(void, copyConstructor, ());
    MOCK_METHOD(void, moveConstructor, ());
    MOCK_METHOD(void, copyAssign, ());
    MOCK_METHOD(void, moveAssign, ());
    MOCK_METHOD(void, end, ());
} testClsMocks;

struct TestPF
{
    TestPF() {}
    TestPF(const TestPF&) { testClsMocks.copyConstructor(); }
    TestPF(TestPF&&) { testClsMocks.moveConstructor(); }
    TestPF& operator=(const TestPF&) { testClsMocks.copyAssign(); return *this; }
    TestPF& operator=(TestPF&&) { testClsMocks.moveAssign(); return *this; }
};

struct TestEnd
{
    ~TestEnd() { testClsMocks.end(); }
};

struct TestCls
{
    static std::set<TestCls*> instances;
    TestCls() { instances.insert(this); testClsMocks.constructor(); }
    TestCls(float a, std::string b) { instances.insert(this); testClsMocks.constructor(a, b); }
    TestCls(const TestPF&) { testClsMocks.constructorWithPF(); }
    TestCls(int, TestPF) { testClsMocks.constructorWithPF(); }
    ~TestCls() { testClsMocks.destructor(this); instances.erase(this); }
};

std::set<TestCls*> TestCls::instances;


TEST(Ref, CreateDelete) {
    {
        testing::InSequence seq;
        EXPECT_CALL(testClsMocks, constructor());
        Ref<TestCls> ref = Ref<TestCls>::make();
        EXPECT_CALL(testClsMocks, destructor(testing::_));
    }
    {
        testing::InSequence seq;
        EXPECT_CALL(testClsMocks, constructor(1.2f, "test"));
        Ref<TestCls> ref = Ref<TestCls>::make(1.2f, "test");
        EXPECT_CALL(testClsMocks, destructor(testing::_));
    }
    {
        testing::InSequence seq;
        EXPECT_CALL(testClsMocks, constructorWithPF());
        Ref<TestCls> ref = Ref<TestCls>::make(TestPF());
        EXPECT_CALL(testClsMocks, destructor(testing::_));
    }
    {
        testing::InSequence seq;
        EXPECT_CALL(testClsMocks, moveConstructor());
        EXPECT_CALL(testClsMocks, constructorWithPF());
        Ref<TestCls> ref = Ref<TestCls>::make(1, TestPF());
        EXPECT_CALL(testClsMocks, destructor(testing::_));
    }
}


TEST(Ref, CopyAssign) {
    testing::InSequence seq;
    TestEnd end;
    EXPECT_CALL(testClsMocks, constructor())
        .Times(2);
    Ref<TestCls> a = Ref<TestCls>::make();
    Ref<TestCls> b = Ref<TestCls>::make();
    TestCls* pa = a.ptr;
    TestCls* pb = b.ptr;
    EXPECT_CALL(testClsMocks, destructor(pa));
    a = b;
    EXPECT_EQ(((uint32_t*)pb)[-1], 2);
    EXPECT_CALL(testClsMocks, destructor(pb));
    EXPECT_CALL(testClsMocks, end());
}


TEST(Ref, MoveAssign) {
    testing::InSequence seq;
    TestEnd end;
    EXPECT_CALL(testClsMocks, constructor());
    Ref<TestCls> a = Ref<TestCls>::make();
    TestCls* pa = a.ptr;
    EXPECT_CALL(testClsMocks, constructor());
    EXPECT_CALL(testClsMocks, destructor(pa));
    a = Ref<TestCls>::make();
    pa = a.ptr;
    EXPECT_EQ(((uint32_t*)pa)[-1], 1);
    EXPECT_CALL(testClsMocks, destructor(pa));
    EXPECT_CALL(testClsMocks, end());
}


TEST(Ref, CopyConstr) {
    testing::InSequence seq;
    TestEnd end;
    EXPECT_CALL(testClsMocks, constructor());
    Ref<TestCls> b = Ref<TestCls>::make();
    Ref<TestCls> a(b);
    TestCls* pb = b.ptr;
    EXPECT_EQ(((uint32_t*)pb)[-1], 2);
    b = nullptr;
    EXPECT_EQ(((uint32_t*)pb)[-1], 1);
    EXPECT_CALL(testClsMocks, destructor(pb));
    a = nullptr;
    EXPECT_CALL(testClsMocks, end());
}


TEST(Ref, MoveConstr) {
    testing::InSequence seq;
    TestEnd end;
    EXPECT_CALL(testClsMocks, constructor());
    Ref<TestCls> a(Ref<TestCls>::make());
    TestCls* pa = a.ptr;
    EXPECT_EQ(((uint32_t*)pa)[-1], 1);
    EXPECT_CALL(testClsMocks, destructor(pa));
    a = nullptr;
    EXPECT_CALL(testClsMocks, end());
}


TEST(Ref, Compare) {
    EXPECT_CALL(testClsMocks, constructor())
        .Times(2);
    EXPECT_CALL(testClsMocks, destructor(testing::_))
        .Times(2);
    Ref<TestCls> a = Ref<TestCls>::make();
    Ref<TestCls> b = Ref<TestCls>::make();
    Ref<TestCls> c = a;
    EXPECT_TRUE(a == c);
    EXPECT_FALSE(b == c);
    EXPECT_TRUE(b != c);
    EXPECT_FALSE(a != c);
    c = nullptr;
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == nullptr);
    EXPECT_TRUE(a != nullptr);
    EXPECT_TRUE(c == nullptr);
    EXPECT_FALSE(nullptr != c);
    EXPECT_FALSE(nullptr == a);
    EXPECT_TRUE(nullptr != a);
    EXPECT_TRUE(nullptr == c);
    EXPECT_FALSE(nullptr != c);
}


TEST(Ref, Unmanaged) {
    testing::InSequence seq;
    TestEnd end;
    EXPECT_CALL(testClsMocks, constructor());
    Ref<TestCls> a = Ref<TestCls>::make();
    TestCls* pa = a.createUnmanaged();
    EXPECT_EQ(((uint32_t*)pa)[-1], 2);
    a = nullptr;
    EXPECT_EQ(((uint32_t*)pa)[-1], 1);
    a = Ref<TestCls>::restoreFromUnmanaged(pa);
    EXPECT_EQ(((uint32_t*)pa)[-1], 2);
    Ref<TestCls>::deleteUnmanaged(pa);
    EXPECT_EQ(((uint32_t*)pa)[-1], 1);
    EXPECT_CALL(testClsMocks, destructor(testing::_));
    a = nullptr;
    EXPECT_CALL(testClsMocks, constructor());
    a = Ref<TestCls>::make();
    pa = a.createUnmanaged();
    a = nullptr;
    EXPECT_CALL(testClsMocks, destructor(testing::_));
    Ref<TestCls>::deleteUnmanaged(pa);
    EXPECT_CALL(testClsMocks, end());
}
