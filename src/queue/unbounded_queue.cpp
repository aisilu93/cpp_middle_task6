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
    cv_ready_to_push.notify_all();
}

void UnboundedQueue::push(std::function<void()> task) {
    if (need_stop.load(std::memory_order_acquire))
        return;

    if (options.bounded) {
        std::unique_lock<std::mutex> cv_lock(mutex_);
        cv_ready_to_push.wait(
            cv_lock, [this]() { return tasks.size() < options.capacity || need_stop.load(std::memory_order_acquire); });
    }

    std::lock_guard<std::mutex> lock(mutex_);
    tasks.emplace(std::move(task));
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tasks.empty())
        return std::nullopt;

    auto task = std::move(tasks.front());
    tasks.pop();

    if (options.bounded)
        cv_ready_to_push.notify_one();

    return task;
}

void UnboundedQueue::shutdown() {
    need_stop.store(true, std::memory_order_release);
    cv_ready_to_push.notify_all();
}

}  // namespace dispatcher::queue