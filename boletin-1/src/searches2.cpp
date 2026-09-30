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
#include "../include/generador.hpp"

// Tipo de puntero a función
using SearchFn = int(*)(const std::vector<int>&, int);

// Validación de CLI
void validate_input_exp2(int argc, char *argv[], std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, 
    std::int64_t& step, std::string& algo) 
{
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] 
            << " <salida.csv> <RUNS> <N_FIXED> <UPPER_DUMMY> <STEP_DUMMY> <ALGORITMO>\n"
            << "Algoritmos válidos: sec, bin, gal, stl_find, stl_bin, stl_lower" 
            << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    validate_input(argc - 1, argv, runs, lower, upper, step);
    algo = argv[6];
}

// Selección del algoritmo antes de la medición
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
    if (algo == "stl_lower") return wrapper_stl_lower_bound;

    std::cerr << "Error: Algoritmo desconocido '" << algo << "'." << std::endl;
    std::exit(EXIT_FAILURE);
}

// Dataset uniforme
std::vector<int> generar_dataset_uniforme(size_t n) {
    std::vector<int> dataset(n);
    for (size_t i = 0; i < n; ++i) {
        dataset[i] = static_cast<int>(i * 2);
    }
    return dataset;
}

// Genera un barrido de 40 posiciones (curva densa al inicio)
std::vector<size_t> generar_posiciones_evaluacion(size_t n, size_t num_puntos = 40) {
    std::vector<size_t> posiciones;
    posiciones.reserve(num_puntos);

    // 1. Escala geométrica para capturar el galope cerca del inicio
    for (size_t i = 0; i < num_puntos / 2; ++i) {
        double factor = static_cast<double>(i) / (num_puntos / 2 - 1);
        size_t idx = static_cast<size_t>(std::pow(10.0, factor * 4.0)) - 1; 
        if (idx < n) posiciones.push_back(idx);
    }

    // 2. Escala lineal descentrada en el resto del arreglo
    size_t start_linear = posiciones.empty() ? 0 : posiciones.back() + 100;
    size_t step_linear = (n - start_linear) / (num_puntos - posiciones.size());
    for (size_t idx = start_linear; idx < n; idx += step_linear) {
        if (idx == n / 2) idx += 3; // Evita n/2 exacto (caso trivial para binaria)
        posiciones.push_back(idx);
    }

    std::sort(posiciones.begin(), posiciones.end());
    posiciones.erase(std::unique(posiciones.begin(), posiciones.end()), posiciones.end());

    return posiciones;
}

int main(int argc, char *argv[]) {
    std::int64_t runs, lower, upper, step;
    std::string algo;

    validate_input_exp2(argc, argv, runs, lower, upper, step, algo);

    SearchFn search_func = seleccionar_algoritmo(algo);

    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;

    std::ofstream time_data(argv[1]);
    time_data << "posicion,algoritmo,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4" << std::endl;

    size_t n = static_cast<size_t>(lower);
    std::vector<int> dataset = generar_dataset_uniforme(n);

    if (dataset.empty()) {
        std::cerr << "Error: No se pudieron generar datos para N = " << n << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<size_t> posiciones = generar_posiciones_evaluacion(n, 40);

    std::int64_t total_runs = runs * posiciones.size();
    std::int64_t executed_runs = 0;

    long long global_checksum = 0;

    std::cout << "\033[0;36mEjecutando Experimento 2 (Medición Unitaria) [N = " << n 
              << ", Algoritmo: " << algo << ", Puntos = " << posiciones.size() << "]...\033[0m" << std::endl;

    for (size_t target_idx : posiciones) {
        int valor_objetivo = dataset[target_idx];

        // A. WARM-UP (Calentamiento de caché previo al reloj)
        {
            for (int w = 0; w < 10; ++w) {
                global_checksum += search_func(dataset, valor_objetivo);
            }
        }

        // B. MEDICIÓN UNITARIA
        mean_time = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            auto begin_time = std::chrono::high_resolution_clock::now();
            
            // Medición unitaria directa sobre el valor objetivo en la posición dada
            int res = search_func(dataset, valor_objetivo);
            
            auto end_time = std::chrono::high_resolution_clock::now();

            global_checksum += res;

            std::chrono::duration<double, std::nano> elapsed_time = end_time - begin_time;
            
            // Registra la duración exacta en nanosegundos
            times[i] = elapsed_time.count();
            mean_time += times[i];
        }

        // C. PROCESAMIENTO ESTADÍSTICO
        mean_time /= runs;
        time_stdev = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            dev = times[i] - mean_time;
            time_stdev += dev * dev;
        }
        time_stdev = std::sqrt(time_stdev / (runs > 1 ? runs - 1 : 1));

        quartiles(times, q);

        time_data << target_idx << "," << algo << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << std::endl;
    }

    std::cout << "\n\033[1;32mExperimento 2 Completado con éxito!\033[0m" << std::endl;
    time_data.close();

    if (global_checksum == -99999999) {
        std::cout << global_checksum << std::endl;
    }

    return 0;
}