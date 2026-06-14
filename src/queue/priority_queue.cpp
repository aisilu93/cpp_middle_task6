#include "queue/priority_queue.hpp"
#include "queue/bounded_queue.hpp"
#include "types.hpp"
#include <atomic>
#include <mutex>
#include <utility>

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(std::vector<std::pair<TaskPriority, QueueOptions>> &options) {
    for (const auto &[priority, option] : options) {
        if (option.bounded)
            queue.try_emplace((int)priority, option.capacity.value());
        else
            queue.try_emplace((int)priority);
    }
}

PriorityQueue::~PriorityQueue() { shutdown(); }

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    if (!need_stop.load(std::memory_order_acquire)) {
        queue[(int)priority].push(std::move(task));
    }
    // разбудим кого-нибудь
    cv_has_tasks.notify_one();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::optional<std::function<void()>> task = std::nullopt;
    while (!task) {
        // обойдем все очереди в порядке убывания приоритета в поисках таски
        for (auto i = (int)TaskPriority::High; i <= (int)TaskPriority::Normal; i++) {
            task = queue[i].try_pop();
            if (task)
                return task;
            else if (i == (int)TaskPriority::High) {
                if (need_stop.load(std::memory_order_acquire)) {
                    return std::nullopt;
                }
            } else if (need_stop.load(std::memory_order_acquire))
                i = 0;  // если очередь вдруг останавливают, а мы уже пропустили high, то вернемся к ней
        }
        if (!task) {
            std::unique_lock<std::mutex> cv_lock(mutex_);
            if (!need_stop.load(std::memory_order_acquire))
                cv_has_tasks.wait(cv_lock);
            else
                break;
        }
    }

    return task;
}

void PriorityQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        need_stop.store(true, std::memory_order_release);
    }

    for (auto i = (int)TaskPriority::High; i <= (int)TaskPriority::Normal; i++)
        queue[i].shutdown();

    cv_has_tasks.notify_all();
}

}  // namespace dispatcher::queue