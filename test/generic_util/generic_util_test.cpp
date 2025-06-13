//
// Created by aoikajitsu on 25-6-13.
//
#include <list>
#include <gtest/gtest.h>
import util;
using namespace eden_virt::util;


TEST(MutexDataTest, DirectValueConstruction) {
    mutex_data<int> md(42);
    auto [lock, data] = md.lock();
    EXPECT_EQ(data, 42);
}

TEST(MutexDataTest, MultiArgumentConstruction) {
    mutex_data<std::vector<int>> md(3, 5);  // vector of three 5s
    auto [lock, data] = md.lock();

    ASSERT_EQ(data.size(), 3);
    for (const auto& val : data) {
        EXPECT_EQ(val, 5);
    }
}

TEST(MutexDataTest, BraceInitialization) {
    mutex_data<std::vector<int>> md{3, 5};
    auto [lock, data] = md.lock();
    ASSERT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 3);
    EXPECT_EQ(data[1], 5);
}

TEST(MutexDataTest, VectorSizeInitialization) {
    mutex_data<std::vector<int>> md(3, 5);
    auto [lock, data] = md.lock();
    ASSERT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 5);
    EXPECT_EQ(data[1], 5);
    EXPECT_EQ(data[2], 5);
}

TEST(MutexDataTest, ComplexType) {
    struct Point {
        int x, y;
        Point(int x, int y) : x(x), y(y) {}
    };

    mutex_data<Point> point(3, 4);
    auto [lock, data] = point.lock();

    EXPECT_EQ(data.x, 3);
    EXPECT_EQ(data.y, 4);
}

TEST(MutexDataTest, LockScope) {
    mutex_data<int> value(0);

    {
        auto [lock, data] = value.lock();
        data = 42;
    }  // Lock released here

    // Verify the lock was released
    bool lock_acquired = false;
    std::thread t([&] {
        auto [lock, data] = value.lock();
        lock_acquired = true;
        EXPECT_EQ(data, 42);
    });

    t.join();
    EXPECT_TRUE(lock_acquired);
}

TEST(MutexDataTest, ThreadSafety) {
    mutex_data<int> counter(0);
    constexpr int kNumThreads = 10;
    constexpr int kIncrementsPerThread = 1000;

    std::vector<std::thread> threads;

    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&counter] {
            for (int j = 0; j < kIncrementsPerThread; ++j) {
                auto [lock, data] = counter.lock();
                ++data;
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }

    auto [lock, final_count] = counter.lock();
    EXPECT_EQ(final_count, kNumThreads * kIncrementsPerThread);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
