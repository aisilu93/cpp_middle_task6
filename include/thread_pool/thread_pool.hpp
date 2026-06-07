#pragma once
#include <thread>

#include "queue/priority_queue.hpp"
namespace dispatcher::thread_pool {

class ThreadPool {
    size_t pool_size = 0;
    std::vector<std::jthread> pool;
    std::shared_ptr<dispatcher::queue::PriorityQueue> queue;

    void start();
    void stop();

public:
    explicit ThreadPool(std::shared_ptr<dispatcher::queue::PriorityQueue> queue, int pool_size);
    ~ThreadPool();

    bool worker();
};

}  // namespace dispatcher::thread_pool
