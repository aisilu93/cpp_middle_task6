#include "task_dispatcher.hpp"
#include "queue/priority_queue.hpp"
#include <memory>

namespace dispatcher {

TaskDispatcher::TaskDispatcher(size_t thread_count, std::vector<std::pair<TaskPriority, queue::QueueOptions>> &options)
    : queue(std::make_shared<queue::PriorityQueue>(options)), pool(queue, thread_count) {
    pool.start();
}

TaskDispatcher::~TaskDispatcher() { pool.stop(); }

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    queue->push(priority, std::move(task));
}

}  // namespace dispatcher