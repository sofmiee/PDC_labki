#include <iostream>
#include <boost/thread.hpp>
#include <thread>
#include <queue>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>

//небезопасная очередь
class UnsafeQueue {
private:
    std::queue<int> tasks;

public:
    void push(int task) { //добавление задачи в очередь
        tasks.push(task);
    }

    bool pop(int& task) { //вытаскивание задачи из очереди
        if (tasks.empty()) return false;
        task = tasks.front();
        tasks.pop();
        return true;
    }

    size_t size() {
        return tasks.size();
    }
};

//очередь с мьютексом
class MutexQueue {
private:
    std::queue<int> tasks;
    std::mutex mtx;

public:
    //добавление задачи
    void push(int task) {
        std::lock_guard<std::mutex> lock(mtx); //замок, который закрыт во время доавбления задачи
        tasks.push(task);
    }

    //извлечение задачи
    bool pop(int& task) {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) return false;
        task = tasks.front();
        tasks.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.size();
    }
};

//очередь с атомиком
class AtomicQueue {
private:
    std::queue<int> tasks;
    std::mutex mtx;
    std::atomic<size_t> counter{0};

public:
    void push(int task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(task);
        }
        counter++; //неделимая операция увеличения счетчика
    }

    bool pop(int& task) {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) return false;
        task = tasks.front();
        tasks.pop();
        counter--;
        return true;
    }

    size_t size() {
        return counter.load();
    }
};

//производитель
template<typename QueueType> //шаблон создания функции для очереди
void producer(QueueType& q, int producer_id, int tasks_count, std::atomic<long long>& total_pushed) {
    for (int i = 0; i < tasks_count; i++) {
        int task = producer_id * 10000 + i; //создаем отдельные диапазоны задач для каждого потока, чтобы контролировать источник задач
        q.push(task);
        total_pushed++;
        }
    }

//потребитель
template<typename QueueType>
void consumer(QueueType& q, std::atomic<long long>& total_sum, std::atomic<bool>& done, std::atomic<int>& items_processed) {
    int task;
    while (!done) {
        if (q.pop(task)) {
            total_sum += task;
            items_processed++;
        } else {
            // если очередь пуста, немного ждем
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    //дообрабатываем остатки
    while (q.pop(task)) {
        total_sum += task;
        items_processed++;
    }
}

//тест
template<typename QueueType>
void test_queue(const std::string& queue_name, int producers, int consumers, int tasks_per_producer) {

    std::cout << "\n" << queue_name << std::endl;
    std::cout << "Производителей: " << producers << ", Потребителей: " << consumers << std::endl;

    QueueType queue;
    std::atomic<long long> total_pushed{0};
    std::atomic<long long> total_sum{0};
    std::atomic<bool> done{false};
    std::atomic<int> items_processed{0};

    auto start_time = std::chrono::high_resolution_clock::now();

    //запуск происзводителей
    std::vector<boost::thread> producer_threads;
    for (int i = 0; i < producers; i++) {
        producer_threads.emplace_back(producer<QueueType>, std::ref(queue), i, tasks_per_producer, std::ref(total_pushed));
    }

    //запуск потребителей
    std::vector<boost::thread> consumer_threads;
    for (int i = 0; i < consumers; i++) {
        consumer_threads.emplace_back(consumer<QueueType>, std::ref(queue), std::ref(total_sum), std::ref(done), std::ref(items_processed));
    }

    //ждем производителей
    for (auto& t : producer_threads) {
        t.join();
    }

    //задержка для потребителей
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    done = true;

    //ждем потребителей
    for (auto& t : consumer_threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);


    std::cout << "Время: " << duration.count() << " мс \n";
    std::cout << "Добавлено: " << total_pushed.load() << std::endl;
    std::cout << "Обработано: " << items_processed.load() << std::endl;
    std::cout << "Осталось: " << queue.size() << std::endl;

    //проверка потерь
    if (total_pushed.load() == items_processed.load() + queue.size()) { //в очереди могло что то остаться
        std::cout << "Без потерь\n";
    } else {
        std::cout << "Потери:  " << (total_pushed.load() - items_processed.load() - queue.size()) << " задач\n";
    }
}


int main() {



     test_queue<UnsafeQueue>("UNSAFE QUEUE", 4, 4, 1000);


     test_queue<MutexQueue>("MUTEX QUEUE", 4, 4, 1000);



    //test_queue<AtomicQueue>("ATOMIC QUEUE", 4, 4, 1000);
    
    return 0;
}