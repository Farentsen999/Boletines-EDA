#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>
#include <string>
#include <random>
#include <algorithm>

#include "../include/searches.hpp"
#include "../include/uhr_utils.hpp"
//#include "../include/generador.hpp"

// Tipo de puntero a función para evitar comparaciones de string dentro del reloj
using SearchFn = int(*)(const std::vector<int>&, int);

// Validación de argumentos de CLI
void validate_input_exp1(int argc, char *argv[], std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, 
    std::int64_t& step, std::string& algo) 
{
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] 
            << " <salida.csv> <RUNS> <LOWER> <UPPER> <STEP> <ALGORITMO>\n"
            << "Algoritmos válidos: sec, bin, gal, stl_find, stl_bin, stl_lower\n"
            << "<STEP> debe ser >= 2 para multiplicaciones geométricas de n." 
            << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    validate_input(argc - 1, argv, runs, lower, upper, step);
    algo = argv[6];

    if (step < 2) {
        std::cerr << "Error: <STEP> debe ser >= 2 para evitar bucles infinitos en n *= step." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

// Selección del algoritmo antes del cronómetro
SearchFn seleccionar_algoritmo(const std::string& algo) {
    if (algo == "sec") return busqueda_secuencial;
    if (algo == "bin") return [](const std::vector<int>& arr, int target) {
        return busqueda_binaria(arr, target, 0, static_cast<int>(arr.size()) - 1);
    };
    if (algo == "gal") return [](const std::vector<int>& arr, int target) {
        return busqueda_galopante(arr, target, 0, static_cast<int>(arr.size()) - 1);
    };
    if (algo == "stl_find") return wrapper_stl_secuencial;
    if (algo == "stl_bin") return wrapper_stl_binaria;
    if (algo == "stl_lower") return wrapper_stl_galopante;

    std::cerr << "Error: Algoritmo desconocido '" << algo << "'." << std::endl;
    std::exit(EXIT_FAILURE);
}

// Dataset uniforme de enteros pares (0, 2, 4, 6...)
std::vector<int> generar_dataset_uniforme(size_t n) {
    std::vector<int> dataset(n);
    for (size_t i = 0; i < n; ++i) {
        dataset[i] = static_cast<int>(i * 2);
    }
    return dataset;
}

// Genera K claves aleatorias (80% presentes / 20% ausentes)
std::vector<int> generar_claves_aleatorias(const std::vector<int>& dataset, size_t k, std::mt19937& gen) {
    std::vector<int> targets(k);
    std::uniform_int_distribution<size_t> dist_idx(0, dataset.size() - 1);
    std::bernoulli_distribution dist_presente(0.8);

    for (size_t i = 0; i < k; ++i) {
        if (dist_presente(gen)) {
            targets[i] = dataset[dist_idx(gen)];
        } else {
            targets[i] = dataset[dist_idx(gen)] + 1; // Clave ausente (impar)
        }
    }
    return targets;
}

int main(int argc, char *argv[]) {
    std::int64_t runs, lower, upper, step;
    std::string algo;
    
    validate_input_exp1(argc, argv, runs, lower, upper, step, algo);

    SearchFn search_func = seleccionar_algoritmo(algo);

    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;

    std::ofstream time_data(argv[1]);
    time_data << "n,algoritmo,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4" << std::endl;

    std::int64_t total_pasos_n = 0;
    for (std::int64_t temp = lower; temp <= upper; temp *= step) {
        total_pasos_n++;
    }
    std::int64_t total_runs = runs * total_pasos_n;
    std::int64_t executed_runs = 0;

    std::mt19937 gen(42); 
    const size_t K_SEARCHES = 1000; 

    // Acumulador global para engañar al optimizador del compilador
    long long global_checksum = 0;

    std::cout << "\033[0;36mEjecutando Experimento 1 [" << algo << "]...\033[0m" << std::endl;

    for (std::int64_t n = lower; n <= upper; n *= step) {
        std::vector<int> dataset = generar_dataset_uniforme(static_cast<size_t>(n));
        if (dataset.empty()) continue;

        // A. WARM-UP
        {
            std::vector<int> warmup_targets = generar_claves_aleatorias(dataset, 10, gen);
            for (int t : warmup_targets) {
                global_checksum += search_func(dataset, t);
            }
        }

        // B. MEDICIÓN
        mean_time = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            std::vector<int> targets = generar_claves_aleatorias(dataset, K_SEARCHES, gen);

            auto begin_time = std::chrono::high_resolution_clock::now();
            
            long long run_checksum = 0;
            for (size_t k = 0; k < K_SEARCHES; ++k) {
                run_checksum += search_func(dataset, targets[k]);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();

            global_checksum += run_checksum;

            std::chrono::duration<double, std::nano> elapsed_time = end_time - begin_time;
            
            times[i] = elapsed_time.count() / static_cast<double>(K_SEARCHES);
            mean_time += times[i];
        }

        // C. ESTADÍSTICAS
        mean_time /= runs;
        time_stdev = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            dev = times[i] - mean_time;
            time_stdev += dev * dev;
        }
        time_stdev = std::sqrt(time_stdev / (runs > 1 ? runs - 1 : 1));

        quartiles(times, q);

        time_data << n << "," << algo << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << std::endl;
    }

    std::cout << "\n\033[1;32mExperimento 1 Completado con éxito!\033[0m" << std::endl;
    time_data.close();

    // Uso dummy del checksum para evitar que el compilador optimice
    if (global_checksum == -99999999) {
        std::cout << global_checksum << std::endl;
    }

    return 0;
}