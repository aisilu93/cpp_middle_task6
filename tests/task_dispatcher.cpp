#include <atomic>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>

#include "task_dispatcher.hpp"
using namespace dispatcher;

using TP = dispatcher::TaskPriority;

queue::QueueOptions opt3 = {true, 4};
queue::QueueOptions opt4 = {false, std::nullopt};

std::vector<std::pair<TP, queue::QueueOptions>> options = {{TP::High, opt3}, {TP::Normal, opt4}};

TEST(DispatcherTest, OneTaskExecution) {
    std::atomic<bool> executed(false);
    {
        TaskDispatcher dispatcher(1, options);
        auto f = [&] { executed.store(true, std::memory_order_release); };
        dispatcher.schedule(TP::High, f);
    }
    sleep(1);
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST(DispatcherTest, ManyTaskExecution) {
    std::atomic<int> count = 0;
    TaskDispatcher dispatcher(1, options);
    auto f = [&] { count++; };
    for (int i = 0; i < 10; i++)
        dispatcher.schedule(TP::Normal, f);
    sleep(5);
    // EXPECT_EQ(count, 100);
}

TEST(DispatcherTest, ExecAfterDestruction) {
    std::atomic<int> count = 0;
    {
        TaskDispatcher dispatcher(1, options);
        auto f = [&] { count++; };
        for (int i = 0; i < 3; i++)
            dispatcher.schedule(TP::High, f);
    }
    sleep(1);
    EXPECT_EQ(count, 3);
}

TEST(DispatcherTest, TaskException) {
    std::atomic<bool> executed(false);
    {
        TaskDispatcher dispatcher(1, options);
        auto f1 = [] { throw std::runtime_error("test exceptions"); };
        auto f2 = [&] { executed.store(true, std::memory_order_release); };

        dispatcher.schedule(TP::Normal, f1);
        dispatcher.schedule(TP::Normal, f2);
        sleep(1);
    }
    sleep(1);
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST(DispatcherTest, ManyPoolThreads) {
    std::map<std::jthread::id, std::atomic<int>> threads_counter;
    int pushed_total = 100;
    {
        TaskDispatcher dispatcher(std::jthread::hardware_concurrency(), options);
        auto f = [&] {
            auto id = std::this_thread::get_id();
            threads_counter[id]++;
        };
        for (int i = 0; i < pushed_total; i++)
            dispatcher.schedule(TP::Normal, f);
        sleep(5);
    }
    sleep(1);
    EXPECT_EQ(threads_counter.size(), std::jthread::hardware_concurrency());
    int executed_total = 0;
    for (const auto &[_, count] : threads_counter)
        executed_total += count;

    EXPECT_EQ(pushed_total, executed_total);
}
