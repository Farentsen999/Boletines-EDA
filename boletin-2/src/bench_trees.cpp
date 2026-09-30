#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <string>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <cassert>

// Inclusión de los encabezados de las estructuras corregidas
#include "../include/avlTree.hpp"
#include "../include/splayTree.hpp"
#include "../include/redBlackTree.hpp"

// ============================================================================
// GENERADOR DE DATOS Y DISTRIBUCIONES (Reemplazo de generador.hpp)
// ============================================================================

// Genera un vector de elementos únicos
std::vector<int> generateRandomDataset(size_t size, unsigned int seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(1, static_cast<int>(size * 10));
    std::vector<int> data;
    data.reserve(size);

    std::set<int> unique_vals;
    while (unique_vals.size() < size) {
        unique_vals.insert(dist(gen));
    }
    data.assign(unique_vals.begin(), unique_vals.end());
    std::shuffle(data.begin(), data.end(), gen);
    return data;
}

// Crea una distribución discreta tipo Zipf / Powerlaw (80/20 o alpha parametrizable)
std::discrete_distribution<size_t> make_zipf_distribution(size_t n, double alpha) {
    std::vector<double> weights(n);
    for (size_t i = 0; i < n; ++i) {
        weights[i] = 1.0 / std::pow(static_cast<double>(i + 1), alpha);
    }
    return std::discrete_distribution<size_t>(weights.begin(), weights.end());
}

// ============================================================================
// EXPERIMENTO 1: BUSQUEDA CON DISTRIBUCION SESGADA (ZIPF / POWERLAW)
// ============================================================================
void run_powerlaw_experiment(size_t n, int runs, double alpha) {
    std::cout << "\n=== Experimento: Búsquedas con Distribución Zipf (Alpha: " << alpha << ") ===\n";
    std::cout << "N = " << n << ", Repeticiones = " << runs << "\n";

    double total_time_avl = 0.0;
    double total_time_splay = 0.0;
    double total_time_rb = 0.0;

    size_t num_queries = 100000;

    for (int r = 0; r < runs; ++r) {
        unsigned int seed = 42 + r;
        auto dataset = generateRandomDataset(n, seed);

        AVLTree<int> avl;
        SplayTree<int> splay;
        RedBlackTree<int> rb;

        for (int val : dataset) {
            avl.insert(val);
            splay.insert(val);
            rb.insert(val);
        }

        std::mt19937 gen(seed + 100);
        auto zipf = make_zipf_distribution(n, alpha);

        // Generar orden de accesos sesgados
        std::vector<int> search_keys(num_queries);
        for (size_t i = 0; i < num_queries; ++i) {
            search_keys[i] = dataset[zipf(gen)];
        }

        // Calentamiento previo para Splay (Warm-up)
        for (size_t i = 0; i < 5000; ++i) {
            splay.search(search_keys[i]);
        }

        // Medición AVL
        auto start = std::chrono::high_resolution_clock::now();
        for (int k : search_keys) avl.search(k);
        auto end = std::chrono::high_resolution_clock::now();
        total_time_avl += std::chrono::duration<double, std::micro>(end - start).count();

        // Medición Splay
        start = std::chrono::high_resolution_clock::now();
        for (int k : search_keys) splay.search(k);
        end = std::chrono::high_resolution_clock::now();
        total_time_splay += std::chrono::duration<double, std::micro>(end - start).count();

        // Medición RedBlack
        start = std::chrono::high_resolution_clock::now();
        for (int k : search_keys) rb.search(k);
        end = std::chrono::high_resolution_clock::now();
        total_time_rb += std::chrono::duration<double, std::micro>(end - start).count();
    }

    double total_ops = static_cast<double>(num_queries * runs);
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Tiempo promedio por operación (microsegundos):\n";
    std::cout << "  AVL:       " << total_time_avl / total_ops << " us/op\n";
    std::cout << "  Splay:     " << total_time_splay / total_ops << " us/op\n";
    std::cout << "  RedBlack:  " << total_time_rb / total_ops << " us/op\n";
}

// ============================================================================
// EXPERIMENTO 2: LOCALIDAD DE ACCESO Y EVOLUCION TEMPORAL
// ============================================================================
void run_locality_experiment(size_t n, int repetitions, int consecutive_searches) {
    std::cout << "\n=== Experimento: Localidad Temporal de Acceso (Evolución por búsqueda) ===\n";
    std::cout << "N = " << n << ", Búsquedas consecutivas del mismo elemento = " << consecutive_searches << "\n";

    std::vector<double> avl_times(consecutive_searches, 0.0);
    std::vector<double> splay_times(consecutive_searches, 0.0);

    for (int r = 0; r < repetitions; ++r) {
        unsigned int seed = 1000 + r;
        auto dataset = generateRandomDataset(n, seed);

        AVLTree<int> avl;
        SplayTree<int> splay;
        for (int val : dataset) {
            avl.insert(val);
            splay.insert(val);
        }

        // Seleccionar una clave que esté a cierta profundidad inicial
        int target_key = dataset[n / 2];

        // Medición paso a paso del elemento objetivo
        for (int step = 0; step < consecutive_searches; ++step) {
            auto start = std::chrono::high_resolution_clock::now();
            avl.search(target_key);
            auto end = std::chrono::high_resolution_clock::now();
            avl_times[step] += std::chrono::duration<double, std::nano>(end - start).count();

            start = std::chrono::high_resolution_clock::now();
            splay.search(target_key);
            end = std::chrono::high_resolution_clock::now();
            splay_times[step] += std::chrono::duration<double, std::nano>(end - start).count();
        }
    }

    std::cout << "Paso | AVL (ns) | Splay (ns)\n";
    std::cout << "---------------------------\n";
    for (int i = 0; i < consecutive_searches; ++i) {
        std::cout << std::setw(4) << i + 1 << " | "
                  << std::setw(8) << std::fixed << std::setprecision(1) << avl_times[i] / repetitions << " | "
                  << std::setw(10) << splay_times[i] / repetitions << "\n";
    }
}

// ============================================================================
// EXPERIMENTO 3: CONTROL / BASELINE (Misma Clave vs Claves Distintas)
// ============================================================================
void run_control_experiment(size_t n, int k_ops) {
    std::cout << "\n=== Experimento Control (Baseline): Repetición Mismo Elemento vs Aleatorios ===\n";
    
    unsigned int seed = 42;
    auto dataset = generateRandomDataset(n, seed);

    SplayTree<int> splay_same;
    SplayTree<int> splay_diff;

    for (int val : dataset) {
        splay_same.insert(val);
        splay_diff.insert(val);
    }

    int target_key = dataset[dataset.size() - 1]; // Clave al fondo/lejana

    // Caso A: Misma clave K veces seguidas
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < k_ops; ++i) {
        splay_same.search(target_key);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double time_case_a = std::chrono::duration<double, std::micro>(end - start).count();

    // Caso B: K claves aleatorias distintas
    std::mt19937 gen(123);
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < k_ops; ++i) {
        splay_diff.search(dataset[dist(gen)]);
    }
    end = std::chrono::high_resolution_clock::now();
    double time_case_b = std::chrono::duration<double, std::micro>(end - start).count();

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Total de operaciones: " << k_ops << "\n";
    std::cout << "  Caso A (Misma Clave consecutiva):  " << time_case_a / k_ops << " us/op\n";
    std::cout << "  Caso B (Claves distintas aleatorias): " << time_case_b / k_ops << " us/op\n";
    std::cout << "  Aceleración (Speedup Caso A / Caso B): " << time_case_b / time_case_a << "x\n";
}

// ============================================================================
// MAIN Y PARSING DE ARGUMENTOS
// ============================================================================
void print_usage(const char* prog_name) {
    std::cout << "Uso: " << prog_name << " <modo> <n_elementos> <repeticiones> [opcion_extra]\n";
    std::cout << "Modos disponibles:\n";
    std::cout << "  1 : Experimento Powerlaw / Zipf (opcion_extra = alpha, default 1.5)\n";
    std::cout << "  2 : Experimento de Localidad Temporal (opcion_extra = pasos consecutivas, default 10)\n";
    std::cout << "  3 : Experimento de Control (Baseline Caso A vs Caso B)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    int mode = std::stoi(argv[1]);
    size_t n = std::stoull(argv[2]);
    int runs = std::stoi(argv[3]);

    switch (mode) {
        case 1: {
            double alpha = (argc >= 5) ? std::stod(argv[4]) : 1.5;
            run_powerlaw_experiment(n, runs, alpha);
            break;
        }
        case 2: {
            int steps = (argc >= 5) ? std::stoi(argv[4]) : 10;
            run_locality_experiment(n, runs, steps);
            break;
        }
        case 3: {
            run_control_experiment(n, runs);
            break;
        }
        default:
            std::cerr << "Modo no reconocido: " << mode << "\n";
            print_usage(argv[0]);
            return 1;
    }

    return 0;
}