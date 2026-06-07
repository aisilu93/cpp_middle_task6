#include "thread_pool/thread_pool.hpp"

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<dispatcher::queue::PriorityQueue> queue, int pool_size) {
    if (pool_size <= 0)
        throw std::invalid_argument("invalid pool size");
    this->pool_size = pool_size;
    this->queue = queue;

    pool.reserve(pool_size);
}

ThreadPool::~ThreadPool() { stop(); }

void ThreadPool::start() {
    for (int i = 0; i < pool_size; i++) {
        pool.emplace_back([this] {
            while (worker()) {
            }
        });
    }
}

void ThreadPool::stop() {
    queue->shutdown();
    for (auto &t : pool) {
        if (t.joinable())
            t.join();
    }
}

bool ThreadPool::worker() {
    auto task = queue->pop();
    if (task) {
        try {
            std::invoke(task.value());
        } catch (const std::exception &e) {
        }
        return true;
    }
    return false;
}

}  // namespace dispatcher::thread_pool