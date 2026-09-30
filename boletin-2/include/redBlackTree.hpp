#ifndef RED_BLACK_TREE_HPP
#define RED_BLACK_TREE_HPP

#include <set>
#include <cstddef>



template <typename T> 
class RedBlackTree {
private:
    std::set<T> rbTree; // La estructura subyacente esta basada en Árbol Rojo-Negro

public:
    //Constructor
    RedBlackTree() = default;

    //Destructor
    ~RedBlackTree() = default;

    /**
     * Inserción de una clave.
     */
    void insert(T key) {
        rbTree.insert(key);
    }

    /*
     * Búsqueda de una clave en el árbol.
     */
    bool search(T key) const {
        return rbTree.find(key) != rbTree.end();
    }

    /**
     * Eliminación de una clave.
     */
    void remove(T key) {
        rbTree.erase(key);
    }

    /**
     * Obtener el número de elementos en el árbol.
     */
    size_t size() const {
        return rbTree.size();
    }

    /**
     * Indica si el arbol esta vacio.
     */
    bool isEmpty() const {
        return rbTree.empty();
    }
};

#endif