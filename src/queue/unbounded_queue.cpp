#include "queue/unbounded_queue.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

UnboundedQueue::UnboundedQueue() {
    options.capacity = 0;
    options.bounded = false;
}

UnboundedQueue::UnboundedQueue(int capacity) {
    if (capacity <= 0)
        throw std::invalid_argument("invalid capacity");

    options.capacity = capacity;
    options.bounded = true;
}
UnboundedQueue::~UnboundedQueue() {
    need_stop.store(true, std::memory_order_release);
    cv_has_tasks.notify_all();
    cv_ready_to_push.notify_all();
}

void UnboundedQueue::push(std::function<void()> task) {
    if (options.bounded) {
        std::unique_lock<std::mutex> cv_lock(mutex_);
        cv_ready_to_push.wait(
            cv_lock, [this]() { return tasks.size() < options.capacity || need_stop.load(std::memory_order_acquire); });
    }

    if (need_stop.load(std::memory_order_acquire))
        return;

    tasks.emplace(std::move(task));
    cv_has_tasks.notify_one();
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    std::unique_lock<std::mutex> cv_lock(mutex_);
    cv_has_tasks.wait(cv_lock, [this]() { return !tasks.empty() || need_stop.load(std::memory_order_acquire); });
    if (need_stop.load(std::memory_order_acquire))
        return std::nullopt;

    auto task = std::move(tasks.front());
    tasks.pop();

    if (options.bounded)
        cv_ready_to_push.notify_one();

    return task;
}

}  // namespace dispatcher::queue