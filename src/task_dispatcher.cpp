#include "task_dispatcher.hpp"
#include "queue/priority_queue.hpp"
#include <memory>

namespace dispatcher {
/*Установите конфигурацию по умолчанию такой:
для задач высокого приоритета очередь ограничена 1000 элементов,
для задач нормального приоритета очередь не ограничена.*/
std::vector<std::pair<TaskPriority, queue::QueueOptions>> options_default = {
    {TaskPriority::High, {true, 1000}}, {TaskPriority::Normal, {false, std::nullopt}}};

TaskDispatcher::TaskDispatcher(size_t thread_count)
    : queue(std::make_shared<queue::PriorityQueue>(options_default)), pool(queue, thread_count) {
    pool.start();
}

TaskDispatcher::TaskDispatcher(size_t thread_count, std::vector<std::pair<TaskPriority, queue::QueueOptions>> &options)
    : queue(std::make_shared<queue::PriorityQueue>(options)), pool(queue, thread_count) {
    pool.start();
}

TaskDispatcher::~TaskDispatcher() { pool.stop(); }

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    queue->push(priority, std::move(task));
}

}  // namespace dispatcher