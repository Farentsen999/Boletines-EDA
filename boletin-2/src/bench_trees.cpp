#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>
#include <string>
#include <random>
#include <numeric>
#include <algorithm>

#include "../include/avlTree.hpp"
#include "../include/redBlackTree.hpp"
#include "../include/splayTree.hpp"
#include "../include/uhr_utils.hpp"
#include "../include/generador.hpp"


void validate_input_trees(int argc, char *argv[], std::int64_t& runs,
    std::int64_t& lower, std::int64_t& upper, 
    std::int64_t& step, std::string& tree_type, std::string& op) 
{
    if (argc != 8) {
        std::cerr << "Uso: " << argv[0] 
        << " <salida.csv> <RUNS> <LOWER> <UPPER> <STEP> <TREE_TYPE> <OPERACION>\n"
        << "TREE_TYPE: avl, rb, splay, splay_powerlaw, splay_locality\n"
        << "OPERACION: insert_all, search_batch, remove_all, search_locality" 
        << std::endl;
        std::exit(EXIT_FAILURE);
    }
    validate_input(argc - 2, argv, runs, lower, upper, step);
    tree_type = argv[6];
    op = argv[7];
}

int main(int argc, char *argv[]) {
    std::int64_t runs, lower, upper, step;
    std::string tree_type, op;

    validate_input_trees(argc, argv, runs, lower, upper, step, tree_type, op);

    std::vector<double> times(runs);
    std::vector<double> q;
    double mean_time, time_stdev, dev;

    std::ofstream time_data(argv[1]);
    time_data << "n,tree,operacion,t_mean,t_stdev,t_Q0,t_Q1,t_Q2,t_Q3,t_Q4" << std::endl;

    std::int64_t total_pasos_n = 0;
    for (std::int64_t temp = lower; temp <= upper; temp *= step) total_pasos_n++;
    std::int64_t total_runs = runs * total_pasos_n;
    std::int64_t executed_runs = 0;

    std::cout << "\033[0;36mEjecutando Benchmark Árboles [" << tree_type << " - " << op << "]...\033[0m" << std::endl;

    std::mt19937 gen(42);

    for (std::int64_t n = lower; n <= upper; n *= step) {
        // Dataset de claves aleatorias para evitar degradación BST
        std::vector<int> dataset = generateRandomDataset(static_cast<size_t>(n));

        mean_time = 0;
        for (std::int64_t r = 0; r < runs; r++) {
            display_progress(++executed_runs, total_runs);

            // ==========================================
            // 1. ÁRBOLES AVL
            // ==========================================
            if (tree_type == "avl") {
                if (op == "insert_all") {
                    AVLTree<int> tree;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.insert(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "search_batch") {
                    AVLTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    const int BATCH = 10000;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < BATCH; ++i) {
                        int target = dataset[i % n];
                        volatile bool res = tree.search(target);
                        (void)res;
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / BATCH;
                } 
                else if (op == "remove_all") {
                    AVLTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.remove(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                }
            } 
            // ==========================================
            // 2. ÁRBOLES ROJO-NEGRO (std::set)
            // ==========================================
            else if (tree_type == "rb") {
                if (op == "insert_all") {
                    RedBlackTree<int> tree;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.insert(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "search_batch") {
                    RedBlackTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    const int BATCH = 10000;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < BATCH; ++i) {
                        int target = dataset[i % n];
                        volatile bool res = tree.search(target);
                        (void)res;
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / BATCH;
                } 
                else if (op == "remove_all") {
                    RedBlackTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.remove(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                }
            } 
            // ==========================================
            // 3. SPLAY TREE (Búsqueda Estándar y Operaciones)
            // ==========================================
            else if (tree_type == "splay") {
                if (op == "insert_all") {
                    SplayTree<int> tree;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.insert(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                } 
                else if (op == "search_batch") {
                    SplayTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    const int BATCH = 10000;
                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < BATCH; ++i) {
                        int target = dataset[i % n];
                        volatile bool res = tree.search(target);
                        (void)res;
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / BATCH;
                } 
                else if (op == "remove_all") {
                    SplayTree<int> tree;
                    for (int val : dataset) tree.insert(val);

                    auto begin = std::chrono::high_resolution_clock::now();
                    for (int val : dataset) tree.remove(val);
                    auto end = std::chrono::high_resolution_clock::now();
                    times[r] = std::chrono::duration<double, std::nano>(end - begin).count();
                }
            }
            // ==========================================
            // 4. SPLAY TREE CON LEY DE POTENCIAS (POWER-LAW / ZIPF)
            // ==========================================
            else if (tree_type == "splay_powerlaw") {
                SplayTree<int> tree;
                for (int val : dataset) tree.insert(val);

                // Se generan 10,000 consultas sesgadas: unos pocos elementos concentran la mayoría de búsquedas
                const int BATCH = 10000;
                double alpha = 1.8; // Parámetro de concentración de la ley de potencia
                
                auto begin = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < BATCH; ++i) {
                    size_t idx = get_powerlaw_index(static_cast<size_t>(n), alpha, gen);
                    int target = dataset[idx];
                    volatile bool res = tree.search(target);
                    (void)res;
                }
                auto end = std::chrono::high_resolution_clock::now();
                
                // Tiempo promedio por búsqueda bajo un patrón Power-Law
                times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / BATCH;
            }
            // ==========================================
            // 5. SPLAY TREE - EXPERIMENTO DE LOCALIDAD REPETIDA
            // ==========================================
            else if (tree_type == "splay_locality") {
                SplayTree<int> tree;
                for (int val : dataset) tree.insert(val);

                // Elegir un elemento ubicado al final del dataset (inicialmente profundo en el árbol)
                int deep_target = dataset[n - 1];
                const int REPETITIONS = 10;

                auto begin = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < REPETITIONS; ++i) {
                    volatile bool res = tree.search(deep_target);
                    (void)res;
                }
                auto end = std::chrono::high_resolution_clock::now();

                // Promedio de tiempo cuando se accede consecutivamente a la misma clave
                times[r] = std::chrono::duration<double, std::nano>(end - begin).count() / REPETITIONS;
            }

            mean_time += times[r];
        }

        // Procesamiento Estadístico
        mean_time /= runs;
        time_stdev = 0;
        for (std::int64_t i = 0; i < runs; i++) {
            dev = times[i] - mean_time;
            time_stdev += dev * dev;
        }
        time_stdev = std::sqrt(time_stdev / (runs > 1 ? runs - 1 : 1));
        quartiles(times, q);

        time_data << n << "," << tree_type << "," << op << "," << mean_time << "," << time_stdev << ",";
        time_data << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << "," << q[4] << std::endl;
    }

    std::cout << "\n\033[1;32mBenchmark Árboles Completado Exitosamente!\033[0m" << std::endl;
    time_data.close();
    return 0;
}