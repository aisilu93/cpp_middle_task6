#include "thread_pool/thread_pool.hpp"
#include <exception>
#include <print>

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<dispatcher::queue::PriorityQueue> queue, int pool_size) {
    if (pool_size <= 0)
        throw std::invalid_argument("invalid pool size");
    this->pool_size = pool_size;
    this->queue = queue;

    pool.reserve(pool_size);
}

ThreadPool::~ThreadPool() {}

void ThreadPool::start() {
    for (int i = 0; i < pool_size; i++) {
        pool.emplace_back([this] { worker(); });
    }
}

void ThreadPool::stop() {
    queue->shutdown();
    for (auto &t : pool) {
        if (t.joinable())
            t.join();
    }
}

void ThreadPool::worker() {
    while (true) {
        auto task = queue->pop();

        if (!task)
            break;

        try {
            std::invoke(*task);
        } catch (std::exception &e) {
            std::print("exception {}", e.what());
        }
    }
}

}  // namespace dispatcher::thread_pool