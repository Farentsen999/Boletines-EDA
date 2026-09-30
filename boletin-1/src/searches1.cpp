
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>
#include <string>

#include "../include/searches.hpp"
#include "../include/uhr_utils.hpp"
#include "../include/generador.hpp"

// Función auxiliar para validar la entrada extendida
void validate_input_exp1(int argc, char *argv[], std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, 
    std::int64_t& step, std::string& algo) 
{
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] 
        << " <salida.csv> <RUNS> <LOWER> <UPPER> <STEP> <ALGORITMO>\n"
        << "Algoritmos válidos: sec, bin, gal, stl_find, stl_bin, stl_lower" 
        << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    validate_input(argc - 1, argv, runs, lower, upper, step);
    algo = argv[6];
}

int main(int argc, char *argv[]) {
    std::int64_t runs, lower, upper, step;
    std::string algo;
    
    validate_input_exp1(argc, argv, runs, lower, upper, step, algo);

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

    std::cout << "\033[0;36mEjecutando Experimento 1 [" << algo << "]...\033[0m" << std::endl;

    for (std::int64_t n = lower; n <= upper; n *= step) {
        // Generación limpia del dataset sin sobreescribir
        std::vector<int> dataset = generatePrimes(static_cast<size_t>(n));
        
        if (dataset.empty()) continue;
        int valor_objetivo = dataset[3*n / 4]; 

        // A. WARM-UP (Llamadas directas para inlining óptimo del compilador)
        for (int w = 0; w < 8; ++w) {
            volatile int res = 0;
            if (algo == "sec") res = busqueda_secuencial(dataset, valor_objetivo);
            else if (algo == "bin") res = busqueda_binaria(dataset, valor_objetivo, 0, n - 1);
            else if (algo == "gal") res = busqueda_galopante(dataset, valor_objetivo, 0, n - 1);
            else if (algo == "stl_find") res = wrapper_stl_secuencial(dataset, valor_objetivo);
            else if (algo == "stl_bin") res = wrapper_stl_binaria(dataset, valor_objetivo);
            else if (algo == "stl_lower") res = wrapper_stl_lower_bound(dataset, valor_objetivo);
            (void)res;
        }

        // B. BUCLE DE MEDICIÓN DE ALTA PRECISIÓN
        mean_time = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            display_progress(++executed_runs, total_runs);

            auto begin_time = std::chrono::high_resolution_clock::now();
            
            // Llamada directa sin abstracciones intermedias
            volatile int res = 0;
            if (algo == "sec") {
                res = busqueda_secuencial(dataset, valor_objetivo);
            } else if (algo == "bin") {
                res = busqueda_binaria(dataset, valor_objetivo, 0, n - 1);
            } else if (algo == "gal") {
                res = busqueda_galopante(dataset, valor_objetivo, 0, n - 1);
            } else if (algo == "stl_find") {
                res = wrapper_stl_secuencial(dataset, valor_objetivo);
            } else if (algo == "stl_bin") {
                res = wrapper_stl_binaria(dataset, valor_objetivo);
            } else if (algo == "stl_lower") {
                res = wrapper_stl_lower_bound(dataset, valor_objetivo);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            (void)res;

            std::chrono::duration<double, std::nano> elapsed_time = end_time - begin_time;
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
        time_stdev = std::sqrt(time_stdev / (runs - 1));

        quartiles(times, q);

        time_data << n << "," << algo << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << std::endl;
    }

    std::cout << "\n\033[1;32mExperimento 1 Completado con éxito!\033[0m" << std::endl;
    time_data.close();
    return 0;
}