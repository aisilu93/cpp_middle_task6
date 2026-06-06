#include <functional>
#include <gtest/gtest.h>
#include <thread>

#include "queue/unbounded_queue.hpp"

using namespace dispatcher::queue;

std::vector<int> v2;

TEST(UnboundedCheck, IncorrectCapacityException) {
    EXPECT_THROW(UnboundedQueue(-1), std::invalid_argument);
    EXPECT_THROW(UnboundedQueue(0), std::invalid_argument);
    EXPECT_NO_THROW(UnboundedQueue());
    EXPECT_NO_THROW(UnboundedQueue(10));
}

TEST(UnboundedCheck, SimplePushAndPop) {
    v2.clear();
    UnboundedQueue queue;

    auto in1 = [&]() { v2.push_back(1); };
    auto in2 = [&]() { v2.push_back(2); };
    auto in3 = [&]() { v2.push_back(3); };

    queue.push(in1);
    queue.push(in2);
    queue.push(in3);

    for (int i = 0; i < 3; i++) {
        auto out = queue.try_pop();
        EXPECT_TRUE(out.has_value());
        std::invoke(out.value());
        EXPECT_EQ(v2.back(), i + 1);
    }
}

TEST(UnboundedCheck, PopFromEmptyReturnsNullopt) {
    UnboundedQueue queue(3);

    std::optional<std::function<void()>> out;
    out = queue.try_pop();
    EXPECT_EQ(out, std::nullopt);
}

TEST(UnboundedCheck, PushToFullBoundedWaits) {
    v2.clear();
    UnboundedQueue queue(1);
    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);

    auto in1 = []() { v2.push_back(1); };
    auto in2 = []() { v2.push_back(2); };
    queue.push(in1);

    std::jthread try_push_thread([&]() {
        wait_start.store(true, std::memory_order_release);
        queue.push(in2);
        wait_end.store(true, std::memory_order_release);
    });

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), false);

    auto out = queue.try_pop();

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), true);

    out = queue.try_pop();
    EXPECT_TRUE(out.has_value());
    std::invoke(out.value());

    EXPECT_EQ(v2.size(), 1);
    EXPECT_EQ(v2[0], 2);
    if (try_push_thread.joinable())
        try_push_thread.join();
}

TEST(UnboundedCheck, StopPushByStopflag) {
    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);
    std::jthread push_thread;
    {
        UnboundedQueue queue(1);
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
