#include "DataPacket.h"
#include <iostream>
#include <queue>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <semaphore>
#include <vector>
#include <atomic>
/*
// глобальные переменные
std::priority_queue<DataPacket> queue;
std::mutex queue_mutex, output_mutex;
std::counting_semaphore<> handler_sem(2);
std::atomic<bool> emergency(false), running(true);
std::atomic<int> active_handlers(2);

const int MAX_QUEUE = 20;
const int LOAD_THRESHOLD = 80;

// отбрасывание низкоприоритетных пакетов при аварии
void drop_low_priority() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::priority_queue<DataPacket> new_q;
    int dropped = 0;

    while (!queue.empty()) {
        auto p = queue.top();
        queue.pop();
        if (emergency && !p.is_critical && p.priority >= 2) dropped++;
        else new_q.push(p);
    }
    queue = new_q;

    if (dropped) {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "[emergency] dropped " << dropped << " packets\n";
    }
}

// монитор нагрузки - добавляет обработчики при загрузке >80%
void monitor() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        int size;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            size = queue.size();
        }

        int load = size * 100 / MAX_QUEUE;

        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[monitor] queue: " << size << "/" << MAX_QUEUE
                      << " (" << load << "%) handlers: " << active_handlers
                      << " emergency: " << (emergency ? "on" : "off") << "\n";
        }

        if (load > LOAD_THRESHOLD && active_handlers < 8) {
            active_handlers++;
            handler_sem.release();
            std::cout << "[monitor] added handler, total: " << active_handlers << "\n";
        }
    }
}

// имитация аварии через 15 секунд после запуска
void emergency_trigger() {
    std::this_thread::sleep_for(std::chrono::seconds(15));

    emergency = true;
    std::cout << "\n!!! emergency mode activated !!!\n";
    drop_low_priority();

    std::this_thread::sleep_for(std::chrono::seconds(8));

    emergency = false;
    std::cout << "\n!!! emergency mode deactivated !!!\n";
}

// станция - генерирует пакеты данных
void station(int id, int packets) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> prio(1, 3), time(500, 2000), delay(500, 3000);
    std::bernoulli_distribution crit(0.2);

    for (int i = 0; i < packets; i++) {
        DataPacket p(i, prio(gen), crit(gen), std::chrono::milliseconds(time(gen)), id);

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue.push(p);
        }

        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[station " << id << "] packet " << i
                      << " (prio=" << p.priority << ", crit=" << p.is_critical << ")\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
    }
}

// обработчик - обрабатывает пакеты из очереди
void handler(int id) {
    while (running) {
        DataPacket p(0, 0, false, std::chrono::milliseconds(0), 0);

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (queue.empty()) {
                if (!running) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            p = queue.top();
            queue.pop();
        }

        handler_sem.acquire();

        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[handler " << id << "] processing packet " << p.id << "\n";
        }

        std::this_thread::sleep_for(p.processing_time);

        std::cout << "[handler " << id << "] done packet " << p.id << "\n";

        handler_sem.release();
    }
}

int main() {
    std::cout << "energy monitoring system\n";

    std::vector<std::thread> handlers, stations;

    // запуск 2 обработчиков
    for (int i = 0; i < 2; i++) handlers.emplace_back(handler, i + 1);
    // запуск монитора и аварии
    std::thread mon(monitor), emerg(emergency_trigger);
    // запуск 10 станций по 8 пакетов
    for (int i = 0; i < 10; i++) stations.emplace_back(station, i + 1, 8);

    // ожидание завершения всех станций
    for (auto& t : stations) t.join();

    std::cout << "\nall stations done, waiting for queue...\n";

    // ожидание опустошения очереди
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (queue.empty()) break;
        std::cout << "queue size: " << queue.size() << "\n";
    }

    // завершение системы
    running = false;
    for (auto& t : handlers) t.join();
    mon.join();
    emerg.join();

    std::cout << "\nsystem shutdown\n";
    return 0;
}*/