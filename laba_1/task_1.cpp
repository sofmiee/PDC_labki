#include <iostream>
#include <boost/thread.hpp>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace boost;
/*
//создаем рандомный массив
vector<int> create_array(int size) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 1000000);

    vector<int> arr(size);
    for (int i = 0; i < size; i++) {
        arr[i] = dist(gen);
    }
    return arr;
}

//сортировка для потока
void sort_part(vector<int>& part) {
    sort(part.begin(), part.end());
}

//соединяем 2 массива
vector<int> merge_two(const vector<int>& a, const vector<int>& b) {
    vector<int> result;
    result.reserve(a.size() + b.size()); //резервируем память для скорости

    int i = 0, j = 0;

    //сливаем пока есть элементы в обоих частях
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            result.push_back(a[i]);
            i++;
        } else {
            result.push_back(b[j]);
            j++;
        }
    }

    //если остались элементы в левом массиве
    while (i < a.size()) {
        result.push_back(a[i]);
        i++;
    }

    //если оставлись элементы в правом массиве
    while (j < b.size()) {
        result.push_back(b[j]);
        j++;
    }

    return result;
}

//соединение всех отсортированных частей слиянием
vector<int> merge_all_parts(const vector<vector<int>>& parts) {
    vector<int> result = parts[0];

    for (int i = 1; i < parts.size(); i++) {
        result = merge_two(result, parts[i]);
    }

    return result;
}

//однопоточная сортировка
void one_thread_sort(const vector<int> &original) {
    vector<int> arr = original; //создаем копию исходного массива

    auto start_time = std::chrono::high_resolution_clock::now(); //фиксируем время в начале
    sort(arr.begin(), arr.end());
    auto end_time = std::chrono::high_resolution_clock::now(); //фиксируем время в конце

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time); //вычисляем время и переводим в миллисекунды

    cout << "Однопоточно: " << duration.count() << " мс" << endl;
}

//многопоточная сортировка
void multi_thread_sort(vector<int> thread_counts, const vector<int> &original) {
    for (int thread_count: thread_counts) {
        vector<int> arr = original; //создаем копию исходного массива

        int part_size = arr.size() / thread_count; //узнаем размер кусочка массива для текущего числа потоков
        vector<vector<int> > parts(thread_count); //разбитый массив

        auto start_time = std::chrono::high_resolution_clock::now(); //отмечаем время начала
        for (int i = 0; i < thread_count; i++) {
            int start = i * part_size;
            int end = start + part_size;

            if (i == thread_count - 1) {
                //если дошли до последнего потока, забираем остаток в него
                end = original.size();
            }

            parts[i].assign(arr.begin() + start, arr.begin() + end);
            //копируем с parts отмеренный кусок массива для iого потока
        }

        //параллельная сортировка частей
        vector<thread> threads;
        for (int i = 0; i < thread_count; i++) {
            threads.emplace_back(sort_part, std::ref(parts[i]));
        }

        //ждем завершения всех потоков
        for (auto &t: threads) {
            t.join();
        }

        //соединяем отсортированные части слиянием
        vector<int> result = merge_all_parts(parts);

        auto end_time = std::chrono::high_resolution_clock::now(); //фиксируем конечно время
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        cout << "Потоков: " << thread_count << endl;
        cout << "Время: " << duration.count() << endl;

    }
}

int main() {
    vector<int> thread_counts = {2, 4, 8};

    cout << "Введите размер массива: ";
    int size;
    cin >> size;

    vector<int> original = create_array(size);//создаем массив

    one_thread_sort(original); //сортируем однопоточно и узнаем время

    //многопоточная сортировка
    multi_thread_sort(thread_counts, original);

    return 0;
} */