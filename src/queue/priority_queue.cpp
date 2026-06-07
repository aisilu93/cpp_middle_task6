#include "queue/priority_queue.hpp"
#include "queue/bounded_queue.hpp"
#include "types.hpp"
#include <atomic>
#include <type_traits>
#include <utility>

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(std::vector<std::pair<TaskPriority, QueueOptions>> &options) {
    for (const auto &[priority, option] : options) {
        if (option.bounded)
            queue.try_emplace(priority, option.capacity.value());
        else
            queue.try_emplace(priority);
    }
}

PriorityQueue::~PriorityQueue() { shutdown(); }

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    if (need_stop.load(std::memory_order_acquire))
        return;
    queue[priority].push(std::move(task));
    tasks_counter.fetch_add(1, std::memory_order_release);
    // добавилась таска, разбудим ждущих
    cv_has_tasks.notify_one();
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    std::optional<std::function<void()>> task = std::nullopt;

    while (true) {
        if (tasks_counter.load(std::memory_order_acquire) > 0) {
            // обойдем все очереди в порядке убывания приоритета в поисках таски
            for (auto i = std::to_underlying(TaskPriority::High); i <= std::to_underlying(TaskPriority::Normal); i++) {
                task = queue[(enum TaskPriority)i].try_pop();
                if (task) {
                    tasks_counter.fetch_sub(1, std::memory_order_release);
                    return task;
                } else if (i == std::to_underlying(TaskPriority::High)) {
                    // если в приоритетной очереди нет задач, проверим, не завершают ли обработку задач
                    if (need_stop.load(std::memory_order_acquire))
                        return task;
                }
            }
        }
        // тасок нет, подождем
        std::unique_lock<std::mutex> cv_lock(mutex_);
        cv_has_tasks.wait(cv_lock, [this]() {
            return tasks_counter.load(std::memory_order_acquire) > 0 || need_stop.load(std::memory_order_acquire);
        });
    }

    return std::nullopt;
}

void PriorityQueue::shutdown() {
    need_stop.store(true, std::memory_order_release);

    for (auto i = std::to_underlying(TaskPriority::High); i <= std::to_underlying(TaskPriority::Normal); i++)
        queue[(enum TaskPriority)i].shutdown();

    cv_has_tasks.notify_all();
}

}  // namespace dispatcher::queue