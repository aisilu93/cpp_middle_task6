#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace dispatcher::queue {

class PriorityQueue {
    std::unordered_map<TaskPriority, UnboundedQueue> queue;

    std::mutex mutex_;
    std::condition_variable cv_has_tasks;
    std::atomic<size_t> tasks_counter = 0;
    std::atomic<bool> need_stop = false;

public:
    explicit PriorityQueue(std::vector<std::pair<TaskPriority, QueueOptions>> &options);

    void push(TaskPriority priority, std::function<void()> task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<std::function<void()>> pop();

    void shutdown();

    ~PriorityQueue();
};

}  // namespace dispatcher::queue