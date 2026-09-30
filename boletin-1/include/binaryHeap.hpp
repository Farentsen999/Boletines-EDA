/* Author: LELE */

#ifndef BINARY_HEAP
#define BINARY_HEAP

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

/** Implementación de un max Binary Heap usando C++'s STL */
class binary_heap
{
    protected:
        std::vector<int> data;

    public:
        /** Constructor para crear un max-heap vacio */
        binary_heap(void)
        {
        }
        
        /** Constructor para crear un max-heap a partir de un vector */
        binary_heap(const std::vector<int> &v) : data(v)
        {
            std::make_heap(data.begin(), data.end());
        }

        /** Retorna la mayor de las claves sin removerla */
        int top(void)
        {
            if (!data.empty())
                return data.front();

            std::cerr << "There's no top: heap is empty." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        /** Inserta una nueva clave */
        void push(int key)
        {
            data.push_back(key);
            std::push_heap(data.begin(), data.end());
        }

        /** Remueve la mayor de las claves*/
        void pop(void)
        {
            std::pop_heap(data.begin(), data.end());
            data.pop_back();
        }

        /** Combina 2 max-Binary Heaps */
        void meld(const binary_heap &h)
        {
            data.insert(data.end(), h.data.begin(), h.data.end());
            std::make_heap(data.begin(), data.end());
        }

        /** Indica si el heap esta o no vacio */
        bool empty(void)
        {
            return data.empty();
        }
};

#endif