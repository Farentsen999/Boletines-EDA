#ifndef SEARCHES_HPP
#define SEARCHES_HPP

#include <vector>
#include <algorithm>
#include <cstddef>

/**
 * Búsqueda Secuencial
 * Retorna el índice del elemento o -1 si no existe.
 */
inline int busqueda_secuencial(const std::vector<int>& arr, int target) {
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] == target) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/**
 * Búsqueda Binaria Nativa (recibe rango low y high)
 * Retorna el índice del elemento o -1 si no existe.
 */
inline int busqueda_binaria(const std::vector<int>& arr, int target, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == target) {
            return mid;
        }
        if (arr[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

/**
 * Búsqueda Galopante (Exponential/Galloping Search)
 * Encuentra el rango donde puede estar el elemento y llama a la búsqueda binaria.
 */
inline int busqueda_galopante(const std::vector<int>& arr, int target, int low, int high) {
    if (arr.empty() || low > high) return -1;

    // Verificar si el primer elemento del rango es el objetivo
    if (arr[low] == target) return low;

    // Determinar el rango mediante saltos exponenciales (galope)
    int bound = 1;
    while (low + bound <= high && arr[low + bound] <= target) {
        bound *= 2;
    }

    // Delimitar los límites para la búsqueda binaria
    int sub_low = low + bound / 2;
    int sub_high = std::min(low + bound, high);

    return busqueda_binaria(arr, target, sub_low, sub_high);
}

// ============================================================================
// 2. WRAPPERS UTILIZANDO LA STL DE C++
// ============================================================================

/**
 * Wrapper de Búsqueda Secuencial mediante std::find
 */
inline int wrapper_stl_secuencial(const std::vector<int>& arr, int target) {
    auto it = std::find(arr.begin(), arr.end(), target);
    if (it != arr.end()) {
        return static_cast<int>(std::distance(arr.begin(), it));
    }
    return -1;
}

/**
 * Wrapper de Búsqueda Binaria mediante std::binary_search
 * Nota: std::binary_search retorna bool, por lo que verificamos la presencia.
 */
inline int wrapper_stl_binaria(const std::vector<int>& arr, int target) {
    bool found = std::binary_search(arr.begin(), arr.end(), target);
    return found ? 1 : -1; // Retorna 1 si existe, -1 si no
}

/**
 * Wrapper de Búsqueda Binaria / Galopante mediante std::lower_bound
 * Retorna el índice exacto si lo encuentra.
 */
inline int wrapper_stl_lower_bound(const std::vector<int>& arr, int target) {
    auto it = std::lower_bound(arr.begin(), arr.end(), target);
    if (it != arr.end() && *it == target) {
        return static_cast<int>(std::distance(arr.begin(), it));
    }
    return -1;
}

#endif