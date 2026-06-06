#pragma once
#include "queue/queue.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
    QueueOptions options;
    std::mutex mutex_;
    std::queue<std::function<void()>> tasks;

    std::condition_variable cv_ready_to_push;
    std::atomic<bool> need_stop = false;

public:
    UnboundedQueue();
    explicit UnboundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    void shutdown();

    ~UnboundedQueue() override;
};

}  // namespace dispatcher::queue