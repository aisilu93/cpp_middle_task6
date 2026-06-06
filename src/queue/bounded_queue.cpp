#include "queue/bounded_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>

namespace dispatcher::queue {

BoundedQueue::BoundedQueue(int capacity) {
    if (capacity <= 0)
        throw std::invalid_argument("invalid capacity");

    options.capacity = capacity;
    options.bounded = true;
}

BoundedQueue::~BoundedQueue() {
    need_stop.store(true, std::memory_order_release);
    cv_has_tasks.notify_all();
    cv_ready_to_push.notify_all();
}

void BoundedQueue::push(std::function<void()> task) {
    std::unique_lock<std::mutex> cv_lock(mutex_);
    cv_ready_to_push.wait(
        cv_lock, [this]() { return tasks.size() < options.capacity || need_stop.load(std::memory_order_acquire); });

    if (need_stop.load(std::memory_order_acquire))
        return;

    tasks.emplace(std::move(task));
    cv_has_tasks.notify_one();
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    std::unique_lock<std::mutex> cv_lock(mutex_);
    cv_has_tasks.wait(cv_lock, [this]() { return !tasks.empty() || need_stop.load(std::memory_order_acquire); });
    if (need_stop.load(std::memory_order_acquire))
        return std::nullopt;

    auto task = std::move(tasks.front());
    tasks.pop();
    cv_ready_to_push.notify_one();

    return task;
}

}  // namespace dispatcher::queue