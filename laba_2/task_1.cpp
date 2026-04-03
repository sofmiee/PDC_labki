#include "Task.h"
#include <queue>
#include <random>
#include <chrono>
#include <semaphore>
#include <mutex>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

std::priority_queue <Task> queue;
std::counting_semaphore <4> processor(4);
std::mutex output_mutex;
std::mutex queue_mutex;

int max_duration = 3000;

std::atomic<bool> processor_alive[4] = {true, true, true, true};
std::atomic<int> active_processors(4);

void split_task() {
    Task task = queue.top();
    if (task.duration > max_duration) {
        for (int i = 0; i < task.duration / max_duration; i++) {
            queue.push(Task{task.id, max_duration, task.priority, task.is_critical});
        }
        if (task.duration % max_duration != 0) {
            queue.push(Task{task.id, task.duration % max_duration, task.priority, task.is_critical});
        }
        queue.pop();
    }
}

void failure_monitor() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_time(3000, 7000);

    std::this_thread::sleep_for(std::chrono::seconds(dis_time(gen)));

    for (int i = 0; i < 4; i++) {
        if (processor_alive[i].load()) {
            processor_alive[i].store(false);
            active_processors--;
            processor.release();

            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "\n!!! PROCESSOR " << (i + 1) << " HAS FAILED !!!\n" << std::endl;
            }
            break;
        }
    }
}

void process_task(int id_thread) {
    while (active_processors.load() > 0) {
        if (!processor_alive[id_thread - 1].load()) {
            return;
        }

        std::unique_lock<std::mutex> queue_lock(queue_mutex);
        if (queue.empty()) {
            return;
        }

        split_task();

        Task task = queue.top();
        queue.pop();
        queue_lock.unlock();

        processor.acquire();

        if (!processor_alive[id_thread - 1].load()) {
            processor.release();
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(task.duration));

        std::lock_guard<std::mutex> output_lock(output_mutex);
        std::cout << "Task №" << task.id << " completed with" << std::endl << " time: " << task.duration << " ms" << std::endl
        << " and priority: " << task.priority << " by processor №" << id_thread << std::endl;
        processor.release();
    }
}

int main() {
    std::vector <std::thread> processors;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis1(1000,7000);
    std::uniform_int_distribution<> dis2(1,10);
    std::uniform_int_distribution<> dis3(0,1);

    for (int i = 0; i < 10; i ++) {
        queue.push(Task{i, dis1(gen),
            dis2(gen), dis3(gen) == 1});
    }

    std::thread monitor(failure_monitor);

    for (int i = 1; i <= 4; i++) {
        processors.push_back(std::thread(process_task, i));
    }

    for (auto& t : processors) {
        t.join();
    }

    monitor.join();
}