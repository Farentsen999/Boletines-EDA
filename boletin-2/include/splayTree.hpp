#include <iostream>

// Adaptación de la implementación de GeeksforGeeks a una clase SplayTree genérica (Template).
template <typename T> 
class SplayNode {
public:
    T key; // Valor o clave guardada en el nodo
    SplayNode* left; // Puntero al hijo izquierdo
    SplayNode* right; // Puntero al hijo derecho
    // Constructor para inicializar un nuevo nodo
    SplayNode(T k)
        : key(k)
        , left(nullptr)
        , right(nullptr)
    {
    }
};


template <typename T> 
class SplayTree {
private:
    // Puntero a la raíz del árbol
    SplayNode<T>* root; 

    // Contador para size()
    size_t node_count;

    /**
     * Rotación a la Derecha:
     * Se usa cuando el subárbol izquierdo está desbalanceado o se requiere elevar un hijo izquierdo.
     */
    SplayNode<T>* rightRotate(SplayNode<T>* x) {
        SplayNode<T>* y = x->left;
        x->left = y->right; // El hijo derecho de 'y' pasa a ser el hijo izquierdo de 'x'
        y->right = x;       // 'x' pasa a ser el hijo derecho de 'y'
        return y;           // 'y' se convierte en la nueva raiz del subárbol
    }

    /**
     * Rotación a la Izquierda (Left Rotate)
     * Se usa cuando el subárbol derecho está desbalanceado o se requiere elevar un hijo derecho.
     */
    SplayNode<T>* leftRotate(SplayNode<T>* x) {
        SplayNode<T>* y = x->right;
        x->right = y->left; // El hijo izquierdo de 'y' pasa a ser el hijo derecho de 'x'
        y->left = x;        // 'x' pasa a ser el hijo izquierdo de 'y'
        return y;           // 'y' se convierte en la nueva raíz del subárbol
    }

    /**
     * Función principal Splay:
     * Busca la clave especificada en el arbol y la desplaza recursivamente a la raíz 
     * mediante una secuencia de rotaciones (Zig, Zig-Zig, Zig-Zag).
     * Si la clave no se encuentra en el árbol, la última clave accedida se convierte en la nueva raíz.
     */
    SplayNode<T>* splay(SplayNode<T>* curr, T key) {
        // Caso base: si el subárbol esta vacío o el nodo actual contiene la clave buscada
        if (curr == nullptr || curr->key == key)
            return curr;

        // Caso 1: La clave se encuentra en el subarbol izquierdo
        if (curr->key > key) {
            // La clave no está presente en el árbol
            if (curr->left == nullptr) return curr;

            // Caso 1a: Zig-Zig (Izquierda - Izquierda)
            if (curr->left->key > key) {
                curr->left->left = splay(curr->left->left, key);
                curr = rightRotate(curr);
            } 
            // Caso 1b: Zig-Zag (Izquierda - Derecha)
            else if (curr->left->key < key) {
                curr->left->right = splay(curr->left->right, key);
                if (curr->left->right != nullptr)
                    curr->left = leftRotate(curr->left); 
            }

            // Segunda rotación a la derecha para elevar el nodo objetivo a la raíz del subárbol
            return (curr->left == nullptr) ? curr : rightRotate(curr);
        } 
        // Caso 2: La clave se encuentra en el subárbol derecho
        else {
            // La clave no está presente en el árbol
            if (curr->right == nullptr) return curr;

            // Caso 2a: Zag-Zig (Derecha - Izquierda)
            if (curr->right->key > key) {
                curr->right->left = splay(curr->right->left, key);
                if (curr->right->left != nullptr)
                    curr->right = rightRotate(curr->right);
            } 
            // Caso 2b: Zag-Zag (Derecha - Derecha)
            else if (curr->right->key < key) {
                curr->right->right = splay(curr->right->right, key);
                curr = leftRotate(curr);
            }

            // Segunda rotación a la izquierda para elevar el nodo objetivo a la raíz del subárbol
            return (curr->right == nullptr) ? curr : leftRotate(curr);
        }
    }

    /**
     * Función auxiliar para liberar la memoria reservada en el Heap.
     */
    void destroyTree(SplayNode<T>* SplayNode) {
        if (SplayNode != nullptr) {
            destroyTree(SplayNode->left);
            destroyTree(SplayNode->right);
            delete SplayNode;
        }
    }

public:
    // Constructor
    SplayTree() : root(nullptr), node_count(0) {}

    // Destructor
    ~SplayTree() {
        destroyTree(root);
    }

    /**
     * Inserción de una clave.
     */
    void insert(T key) {
        // Caso 1: Árbol vacío
        if (root == nullptr) {
            root = new SplayNode<T>(key);
            node_count++;
            return;
        }

        // Mueve la clave más cercana/igual a la raíz
        root = splay(root, key);

        // Si la clave ya existe en el árbol, no se inserta duplicada
        if (root->key == key) return;

        // Se crea el nuevo nodo
        SplayNode<T>* newNode = new SplayNode<T>(key);

        // Ajustar punteros segun la comparación con la nueva raíz
        if (root->key > key) {
            newNode->right = root;
            newNode->left = root->left;
            root->left = nullptr;
        } else {
            newNode->left = root;
            newNode->right = root->right;
            root->right = nullptr;
        }

        root = newNode; // El nuevo nodo se convierte en la raíz principal
        node_count++;
    }

    /**
     * Busqueda de una clave en el árbol.
     */
    bool search(T key) {
        if (root == nullptr) return false;

        // Aplicar splay para elevar la clave (o la clave más cercana) a la raíz
        root = splay(root, key);

        // Si la clave de la nueva raíz coincide, el elemento fue encontrado
        return (root->key == key);
    }

    /**
     * Eliminación de una clave.
     */
    void remove(T key) {
        if (root == nullptr) return;

        // Eleva el nodo a eliminar (o su más cercano) a la raíz
        root = splay(root, key);

        // Si el elemento no existe en el árbol
        if (root->key != key) return;

        SplayNode<T>* temp = root;

        if (root->left == nullptr) {
            root = root->right;
        } else {
            // El máximo del subárbol izquierdo pasa a ser la nueva raíz
            SplayNode<T>* leftSubtree = root->left;
            leftSubtree = splay(leftSubtree, key);
            leftSubtree->right = root->right;
            root = leftSubtree;
        }

        delete temp;
        node_count--;
    }

    /**
     * Obtener el número de elementos en el árbol.
     */
    size_t size() const {
        return node_count;
    }

    /**
     * Indica si el arbol esta vacio.
     */
    bool isEmpty() const {
        return root == nullptr;
    }

};


