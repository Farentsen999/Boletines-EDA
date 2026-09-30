#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>
#include <string>
#include <random>
#include <numeric>
#include <algorithm>

#include "../include/binaryHeap.hpp"
#include "../include/binomialHeap.hpp"
#include "../include/uhr_utils.hpp"
#include "../include/generador.hpp"

void validate_input_heaps(int argc, char *argv[], std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, 
    std::int64_t& step, std::string& heap_type, std::string& op) 
{
    if (argc != 8) {
        std::cerr << "Uso: " << argv[0] 
        << " <salida.csv> <RUNS> <LOWER> <UPPER> <STEP> <HEAP_TYPE> <OPERACION>\n"
        << "HEAP_TYPE: binary, binomial\n"
        << "OPERACION: push_all, top_repeat, pop_all, meld" 
        << std::endl;
        std::exit(EXIT_FAILURE);
    }
    validate_input(argc - 2, argv, runs, lower, upper, step);
    heap_type = argv[6];
    op = argv[7];
}

int main(int argc, char *argv[]) {
    std::int64_t runs, lower, upper, step;
    std::string heap_type, op;

    validate_input_heaps(argc, argv, runs, lower, upper, step, heap_type, op);

    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;

    std::ofstream time_data(argv[1]);
    time_data << "n,heap,operacion,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4" << std::endl;

    std::int64_t total_pasos_n = 0;
    for (std::int64_t temp = lower; temp <= upper; temp *= step) total_pasos_n++;
    std::int64_t total_runs = runs * total_pasos_n;
    std::int64_t executed_runs = 0;

    std::cout << "\033[0;36mEjecutando Benchmark Heaps [" << heap_type << " - " << op << "]...\033[0m" << std::endl;

    for (std::int64_t n = lower; n <= upper; n *= step) {
        std::vector<int> dataset = generateRandomDataset(static_cast<size_t>(n));

        mean_time = 0;
        for (std::int64_t r = 0; r < runs; r++) {
            display_progress(++executed_runs, total_runs);

            if (heap_type == "binary") {
                if (op == "push_all") {
                    // Tiempo total para insertar los N elementos
                    binary_heap bh;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) bh.push(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "top_repeat") {
                    // Repetir top() 10,000 veces
                    binary_heap bh(dataset);
                    const int REPS = 10000;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < REPS; ++i) {
                        volatile int res = bh.top();
                        (void)res;
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    // Tiempo promedio por una operación top()
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / REPS;
                } 
                else if (op == "pop_all") {
                    // Tiempo total para vaciar completamente el heap con N pops
                    binary_heap bh(dataset);
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (std::int64_t i = 0; i < n; ++i) bh.pop();
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "meld") {
                    // Fusionar dos heaps de tamaño N/2
                    std::vector h1_data(dataset.begin(), dataset.begin() + n / 2);
                    std::vector h2_data(dataset.begin() + n / 2, dataset.end());
                    binary_heap h1(h1_data), h2(h2_data);

                    auto begin = std::chrono::high_resolution_clock::now();
                    h1.meld(h2);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                }
            } 
            else if (heap_type == "binomial") {
                if (op == "push_all") {
                    binomial_heap bh;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) bh.insert(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "top_repeat") {
                    binomial_heap bh;
                    for (int val : dataset) bh.insert(val);

                    const int REPS = 1000;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < REPS; ++i) {
                        volatile int res = bh.top();
                        (void)res;
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / REPS;
                } 
                else if (op == "pop_all") {
                    binomial_heap bh;
                    for (int val : dataset) bh.insert(val);

                    auto begin = std::chrono::high_resolution_clock::now();
                    for (std::int64_t i = 0; i < n; ++i) bh.pop();
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "meld") {
                    binomial_heap h1, h2;
                    for (size_t i = 0; i < static_cast<size_t>(n / 2); ++i) h1.insert(dataset[i]);
                    for (size_t i = n / 2; i < dataset.size(); ++i) h2.insert(dataset[i]);

                    auto begin = std::chrono::high_resolution_clock::now();
                    h1.meld(h2); 
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                }
            }
            mean_time += times[r];
        }

        // Estadísticas
        mean_time /= runs;
        time_stdev = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            dev = times[i] - mean_time;
            time_stdev += dev * dev;
        }
        time_stdev = std::sqrt(time_stdev / (runs > 1 ? runs - 1 : 1));
        quartiles(times, q);

        time_data << n << "," << heap_type << "," << op << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << std::endl;
    }

    std::cout << "\n\033[1;32mBenchmark Heaps Completado!\033[0m" << std::endl;
    time_data.close();
    return 0;
}