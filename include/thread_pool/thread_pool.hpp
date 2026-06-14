#pragma once
#include <thread>

#include "queue/priority_queue.hpp"
namespace dispatcher::thread_pool {

class ThreadPool {
    size_t pool_size = 0;
    std::vector<std::jthread> pool;
    std::shared_ptr<dispatcher::queue::PriorityQueue> queue;

    void worker();

public:
    explicit ThreadPool(std::shared_ptr<dispatcher::queue::PriorityQueue> queue, int pool_size);
    ~ThreadPool();
    void start();
    void stop();
};

}  // namespace dispatcher::thread_pool
