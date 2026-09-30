#ifndef GENERADOR_HPP
#define GENERADOR_HPP

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// Genera los primeros 'count' números primos usando Criba con límite dinámico
inline std::vector<int> generatePrimes(size_t count) {
    if (count == 0) return {};

    // Estimación dinámica del límite superior mediante n * ln(n) + n * ln(ln(n))
    size_t limit;
    if (count < 6) {
        limit = 15;
    } else {
        double fn = static_cast<double>(count);
        limit = static_cast<size_t>(fn * (std::log(fn) + std::log(std::log(fn))));
    }

    std::vector<bool> is_prime(limit, true);
    is_prime[0] = is_prime[1] = false;

    for (size_t p = 2; p * p < limit; ++p) {
        if (is_prime[p]) {
            for (size_t i = p * p; i < limit; i += p) {
                is_prime[i] = false;
            }
        }
    }

    std::vector<int> primes;
    primes.reserve(count);
    for (size_t i = 2; i < limit && primes.size() < count; ++i) {
        if (is_prime[i]) {
            primes.push_back(static_cast<int>(i));
        }
    }

    return primes;
}

// Genera los primeros 'count' números compuestos
inline std::vector<int> generateComposites(size_t count) {
    if (count == 0) return {};

    // Estimación dinámica del límite para compuestos
    size_t limit = count * 2 + 100;
    std::vector<bool> is_prime(limit, true);
    is_prime[0] = is_prime[1] = false;

    for (size_t p = 2; p * p < limit; ++p) {
        if (is_prime[p]) {
            for (size_t i = p * p; i < limit; i += p) {
                is_prime[i] = false;
            }
        }
    }

    std::vector<int> composites;
    composites.reserve(count);
    for (size_t i = 4; i < limit && composites.size() < count; ++i) {
        if (!is_prime[i]) {
            composites.push_back(static_cast<int>(i));
        }
    }

    return composites;
}

// Función para generar un dataset de claves aleatorias desordenadas
std::vector<int> generateRandomDataset(size_t n, unsigned int seed = 42) {
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 1); // Llena con 1, 2, ..., n
    std::mt19937 g(seed);
    std::shuffle(data.begin(), data.end(), g);
    return data;
}

// Generador de números según Ley de Potencias (Zipfian Distribution)
// Genera índices entre 0 y n-1 donde los primeros elementos tienen altísima probabilidad
size_t get_powerlaw_index(size_t n, double alpha, std::mt19937& gen) {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double u = dis(gen);
    
    // Transformación aproximada para distribución Power-Law / Zipf
    // alpha > 1 define el nivel de sesgo (ej. 1.5 es un sesgo fuerte)
    double p = std::pow(u, 1.0 / (1.0 - alpha));
    size_t idx = static_cast<size_t>(p * n);
    return std::min(idx, n - 1);
}

#endif // GENERADOR_HPP