#include <functional>
#include <gtest/gtest.h>
#include <optional>
#include <thread>

#include "queue/priority_queue.hpp"
#include "types.hpp"

using namespace dispatcher::queue;
using TP = dispatcher::TaskPriority;

QueueOptions opt1 = {true, 4};
QueueOptions opt2 = {false, std::nullopt};

std::vector<std::pair<TP, QueueOptions>> priorities = {{TP::High, opt1}, {TP::Normal, opt2}};

TEST(PriorityCheck, ReturnByPriorityOrder) {
    PriorityQueue q(priorities);
    std::vector<int> results;
    q.push(TP::Normal, [&]() { results.push_back(1); });
    q.push(TP::Normal, [&]() { results.push_back(1); });
    q.push(TP::High, [&]() { results.push_back(0); });
    q.push(TP::High, [&]() { results.push_back(0); });

    for (int i = 0; i < 4; i++) {
        auto task = q.pop();
        EXPECT_TRUE(task.has_value());
        std::invoke(task.value());
    }
    EXPECT_EQ(results.size(), 4);
    EXPECT_TRUE(results[0] == 0 && results[1] == 0 && results[2] == 1 && results[3] == 1);
}

TEST(PriorityCheck, RightOrderSamePriority) {
    PriorityQueue q(priorities);
    std::vector<int> results;
    q.push(TP::Normal, [&]() { results.push_back(1); });
    q.push(TP::Normal, [&]() { results.push_back(2); });
    q.push(TP::Normal, [&]() { results.push_back(3); });
    q.push(TP::Normal, [&]() { results.push_back(4); });

    for (int i = 0; i < 4; i++) {
        auto task = q.pop();
        EXPECT_TRUE(task.has_value());
        std::invoke(task.value());
        EXPECT_EQ(results.back(), i + 1);
    }
}
TEST(PriorityCheck, PopFromEmptyWaitsPush) {
    PriorityQueue q(priorities);

    bool result = false;
    std::atomic<bool> wait_start(false);
    std::atomic<bool> wait_end(false);
    std::optional<std::function<void()>> out;

    std::jthread t([&]() {
        wait_start.store(true, std::memory_order_release);
        out = q.pop();
        wait_end.store(true, std::memory_order_release);
    });

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), false);
    EXPECT_FALSE(result);

    auto in = [&]() { result = true; };
    q.push(TP::Normal, in);

    sleep(1);
    EXPECT_EQ(wait_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(wait_end.load(std::memory_order_acquire), true);
    EXPECT_TRUE(out.has_value());
    std::invoke(out.value());
    EXPECT_TRUE(result);

    if (t.joinable())
        t.join();
}

TEST(PriorityCheck, Shutdown_PopReturnsOnlyHigh) {
    PriorityQueue q(priorities);
    bool high_invoked = false;
    bool normal_invoked = false;
    q.push(TP::Normal, [&]() { normal_invoked = true; });
    q.push(TP::High, [&]() { high_invoked = true; });
    std::jthread t([&]() { q.shutdown(); });
    auto out = q.pop();
    EXPECT_TRUE(out.has_value());
    std::invoke(out.value());
    EXPECT_TRUE(high_invoked && !normal_invoked);
    out = q.pop();
    EXPECT_FALSE(out.has_value());
}

TEST(PriorityCheck, ShutdownStopsPush) {
    PriorityQueue q(priorities);

    std::atomic<bool> push_start(false);
    std::atomic<bool> push_end(false);
    std::optional<std::function<void()>> out;

    std::jthread t([&]() {
        push_start.store(true, std::memory_order_release);
        for (int i = 0; i != opt1.capacity.value() + 1; i++) {
            q.push(TP::High, [&]() {});
        }
        push_end.store(true, std::memory_order_release);
    });

    sleep(1);
    EXPECT_EQ(push_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(push_end.load(std::memory_order_acquire), false);

    std::jthread t1([&]() { q.shutdown(); });

    sleep(1);

    if (t.joinable())
        t.join();
    if (t1.joinable())
        t1.join();

    EXPECT_EQ(push_start.load(std::memory_order_acquire), true);
    EXPECT_EQ(push_end.load(std::memory_order_acquire), true);
}
