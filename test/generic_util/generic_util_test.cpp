//
// Created by aoikajitsu on 25-6-13.
//
#include <fcntl.h>
#include <list>
#include <thread>
#include <gtest/gtest.h>
import util;
using namespace eden_virt::util;
// This code was generated with the assistance of AI.

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

TEST(SharedMutexDataTest, Construction) {
    shared_mutex_data<std::vector<int>> svec{1, 2, 3};
    auto [lock, data] = svec.shared_lock();
    EXPECT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[2], 3);
}

TEST(SharedMutexDataTest, UniqueLockWrite) {
    shared_mutex_data<int> sdata{10};
    {
        auto [lock, data] = sdata.unique_lock();
        data += 5;
    }
    auto [rlock, data] = sdata.shared_lock();
    EXPECT_EQ(data, 15);
}

TEST(SharedMutexDataTest, MultithreadedReadWrite) {
    shared_mutex_data<int> sdata{0};

    auto reader = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto [lock, data] = sdata.shared_lock();
            EXPECT_GE(data, 0);
        }
    };

    auto writer = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto [lock, data] = sdata.unique_lock();
            data += 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    std::thread r1(reader), r2(reader), w(writer);
    r1.join(); r2.join(); w.join();

    auto [lock, final_value] = sdata.shared_lock();
    EXPECT_EQ(final_value, 100);
}

TEST(FileDescriptorTest, OpenClose) {
    int fd = ::open("/dev/null", O_WRONLY);
    ASSERT_GE(fd, 0);

    file_descriptor f(fd);
    EXPECT_TRUE(f);
    EXPECT_EQ(f.get(), fd);
}

TEST(FileDescriptorTest, MoveSemantics) {
    int fd = ::open("/dev/null", O_WRONLY);
    ASSERT_GE(fd, 0);

    file_descriptor f1(fd);
    file_descriptor f2 = std::move(f1);

    EXPECT_FALSE(f1);
    EXPECT_TRUE(f2);
    EXPECT_EQ(f2.get(), fd);
}

TEST(FileDescriptorTest, Duplicate) {
    int fd = ::open("/dev/null", O_WRONLY);
    ASSERT_GE(fd, 0);

    file_descriptor f(fd);
    file_descriptor dup = f.duplicate();

    EXPECT_TRUE(dup);
    EXPECT_NE(dup.get(), f.get());
    EXPECT_EQ(::write(dup.get(), "a", 1), 1);
}

TEST(FileDescriptorTest, Release) {
    int fd = ::open("/dev/null", O_WRONLY);
    ASSERT_GE(fd, 0);

    file_descriptor f(fd);
    int released_fd = f.release();

    EXPECT_EQ(released_fd, fd);
    EXPECT_FALSE(f);
}

TEST(FileDescriptorTest, ReplaceWith) {
    int fd1 = ::open("/dev/null", O_WRONLY);
    int fd2 = ::dup(fd1);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    file_descriptor f(fd1);
    f.replace_with(fd2);

    EXPECT_FALSE(f);

    EXPECT_EQ(::write(fd2, "a", 1), 1);
    ::close(fd2);
}
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
