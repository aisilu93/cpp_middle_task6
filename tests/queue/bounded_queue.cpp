#include <functional>
#include <gtest/gtest.h>
#include <print>
#include <thread>

#include "queue/bounded_queue.hpp"

using namespace dispatcher::queue;

std::vector<int> v;

TEST(BasicCheck, IncorrectCapacityException) {
    EXPECT_THROW(BoundedQueue(-1), std::invalid_argument);
    EXPECT_THROW(BoundedQueue(0), std::invalid_argument);
}

TEST(BasicCheck, SimplePushAndPop) {
    v.clear();
    BoundedQueue queue(3);

    auto in1 = [&]() { v.push_back(1); };
    auto in2 = [&]() { v.push_back(2); };
    auto in3 = [&]() { v.push_back(3); };

    queue.push(in1);
    queue.push(in2);
    queue.push(in3);

    for (int i = 0; i < 3; i++) {
        auto out = queue.try_pop();
        EXPECT_TRUE(out.has_value());
        std::invoke(out.value());
        EXPECT_EQ(v.back(), i + 1);
    }
}

TEST(BasicCheck, PushToFull) {
    v.clear();
    BoundedQueue queue(3);

    auto in1 = [&]() { v.push_back(1); };
    auto in2 = [&]() { v.push_back(2); };
    auto in3 = [&]() { v.push_back(3); };

    queue.push(in1);
    queue.push(in2);
    queue.push(in3);

    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);

    auto in4 = []() { v.push_back(4); };

    std::jthread try_push_thread([&]() {
        wait_start.store(true, std::memory_order_release);
        queue.push(in4);
        wait_end.store(true, std::memory_order_release);
    });

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), false);

    for (int i = 0; i < 4; i++) {
        auto out = queue.try_pop();
        if (i != 3)
            continue;

        EXPECT_TRUE(out.has_value());
        std::invoke(out.value());
        if (i == 0) {
            EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
            EXPECT_EQ(wait_end.load(std::memory_order_acquire), true);
        }
    }
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v[0], 4);
    if (try_push_thread.joinable())
        try_push_thread.join();
}

TEST(BasicCheck, PopFromEmptyWaits) {
    v.clear();
    BoundedQueue queue(3);

    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);

    std::optional<std::function<void()>> out;
    std::jthread try_pop_thread([&]() {
        wait_start.store(true, std::memory_order_release);
        out = queue.try_pop();
        wait_end.store(true, std::memory_order_release);
    });

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), false);
    EXPECT_FALSE(out.has_value());

    if (try_pop_thread.joinable()) {
        queue.push([]() {});
        try_pop_thread.join();
    }
}

TEST(BasicCheck, StopPushByStopflag) {
    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);
    std::jthread push_thread;
    {
        BoundedQueue queue(1);
        auto in1 = [&]() {};
        auto in2 = [&]() {};
        queue.push(in1);

        std::optional<std::function<void()>> out;
        std::jthread tmp([&]() {
            wait_start.store(true, std::memory_order_release);
            queue.push(in2);
            wait_end.store(true, std::memory_order_release);
        });
        push_thread.swap(tmp);
        sleep(1);
    }

    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), true);
    if (push_thread.joinable()) {
        push_thread.join();
    }
}
