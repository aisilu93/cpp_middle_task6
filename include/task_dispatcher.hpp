#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "queue/queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
    std::shared_ptr<queue::PriorityQueue> queue;
    thread_pool::ThreadPool pool;

public:
    TaskDispatcher(size_t thread_count, std::vector<std::pair<TaskPriority, queue::QueueOptions>> &options);

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();
};

}  // namespace dispatcher