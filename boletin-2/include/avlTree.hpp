#ifndef AVL_TREE_HPP
#define AVL_TREE_HPP

#include <algorithm> 
#include <cstddef>   

// Adaptación de la implementación de GeeksforGeeks a una clase AVL Tree genérica (Template).

// Clase plantilla que representa un nodo dentro del árbol AVL
template <typename T> 
class AVLNode {
public:
    T key;           // Valor o clave almacenada en el nodo
    AVLNode* left;   // Puntero al hijo izquierdo
    AVLNode* right;  // Puntero al hijo derecho
    int height;      // Altura del nodo en el árbol

    // Constructor para inicializar un nodo con una clave dada
    AVLNode(T k)
        : key(k)
        , left(nullptr)
        , right(nullptr)
        , height(1) 
    {
    }
};

// Clase plantilla que representa el Árbol AVL auto-balanceado
template <typename T> 
class AVLTree {
private:
    // Puntero a la raíz del árbol
    AVLNode<T>* root;
    // Contador para size()
    size_t node_count;

    // Función auxiliar para obtener de forma segura la altura de un nodo
    // Retorna 0 si el nodo es nulo (nullptr)
    int height(AVLNode<T>* node)
    {
        if (node == nullptr)
            return 0;
        return node->height;
    }

    // Función auxiliar para calcular el factor de balance o equilibrio de un nodo
    // Factor de balance = Altura(Subárbol Izquierdo) - Altura(Subárbol Derecho)
    int balanceFactor(AVLNode<T>* node)
    {
        if (node == nullptr)
            return 0;
        return height(node->left) - height(node->right);
    }

    // Función auxiliar para realizar una rotación simple a la derecha en un subárbol
    // Se utiliza cuando el subárbol izquierdo está desbalanceado hacia la izquierda (Caso LL)
    AVLNode<T>* rightRotate(AVLNode<T>* y)
    {
        AVLNode<T>* x = y->left;
        AVLNode<T>* T2 = x->right;

        // Realizar la rotación
        x->right = y;
        y->left = T2;

        // Recalcular y actualizar las alturas de los nodos modificados
        y->height = std::max(height(y->left), height(y->right)) + 1;
        x->height = std::max(height(x->left), height(x->right)) + 1;

        // Retornar la nueva raíz del subárbol
        return x;
    }

    // Función auxiliar para realizar una rotación simple a la izquierda en un subárbol
    // Se utiliza cuando el subárbol derecho está desbalanceado hacia la derecha (Caso RR)
    AVLNode<T>* leftRotate(AVLNode<T>* x)
    {
        AVLNode<T>* y = x->right;
        AVLNode<T>* T2 = y->left;

        // Realizar la rotación
        y->left = x;
        x->right = T2;

        // Recalcular y actualizar las alturas de los nodos modificados
        x->height = std::max(height(x->left), height(x->right)) + 1;
        y->height = std::max(height(y->left), height(y->right)) + 1;

        // Retornar la nueva raíz del subárbol
        return y;
    }

    // Función recursiva auxiliar para insertar una nueva clave en el subárbol con raíz en 'node'
    AVLNode<T>* insert(AVLNode<T>* node, T key, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return new AVLNode<T>(key);
        }
        if (key < node->key)
            node->left = insert(node->left, key, inserted);
        else if (key > node->key)
            node->right = insert(node->right, key, inserted);
        else {
            inserted = false; // Clave duplicada
            return node;
        }
        // 2. Actualizar la altura de este nodo ancestro
        node->height = 1 + std::max(height(node->left), height(node->right));

        // 3. Obtener el factor de balance para verificar si se desbalanceó el nodo
        int balance = balanceFactor(node);

        // 4. Si el nodo se desbalancea, se evalúan los 4 casos de rotación:

        // Caso Desbalanceado a la Izquierda (Left Heavy)
        if (balance > 1) {
            // Caso Izquierda-Izquierda (LL)
            if (balanceFactor(node->left) >= 0) {
                return rightRotate(node);
            } 
            // Caso Izquierda-Derecha (LR)
            else {
                node->left = leftRotate(node->left);
                return rightRotate(node);
            }
        }

        // Caso Desbalanceado a la Derecha (Right Heavy)
        if (balance < -1) {
            // Caso Derecha-Derecha (RR)
            if (balanceFactor(node->right) <= 0) {
                return leftRotate(node);
            } 
            // Caso Derecha-Izquierda (RL)
            else {
                node->right = rightRotate(node->right);
                return leftRotate(node);
            }
        }

        return node; // Retornar el puntero del nodo (sin cambios)
    }

    // Función auxiliar para encontrar el nodo con la clave mínima en un subárbol
    // (se usa en la eliminación para encontrar el sucesor In-Order)
    AVLNode<T>* minValueNode(AVLNode<T>* node)
    {
        AVLNode<T>* current = node;
        while (current->left != nullptr)
            current = current->left;
        return current;
    }

    // Función auxiliar para eliminar una clave en el subárbol con raíz en 'root'
    AVLNode<T>* deleteNode(AVLNode<T>* root, T key, bool& deleted) {
        if (root == nullptr) {
            deleted = false;
            return nullptr;
        }
        if (key < root->key)
            root->left = deleteNode(root->left, key, deleted);
        else if (key > root->key)
            root->right = deleteNode(root->right, key, deleted);
        else {
            deleted = true; // Encontrado
            // Caso 1 y 2: Nodo con un solo hijo o sin hijos
            if ((root->left == nullptr) || (root->right == nullptr)) {
                AVLNode<T>* temp = root->left ? root->left : root->right;

                // Caso sin hijos
                if (temp == nullptr) {
                    delete root;
                    return nullptr;
                } else { 
                    // Caso con un hijo: se copian los contenidos y se destruye el nodo antiguo
                    AVLNode<T>* old = root;
                    root = temp;
                    delete old;
                }
            }
            // Caso 3: Nodo con dos hijos
            else {
                // Obtener el sucesor In-Order (el valor mínimo en el subárbol derecho)
                AVLNode<T>* temp = minValueNode(root->right);

                // Copiar la clave del sucesor al nodo actual
                root->key = temp->key;

                // Eliminar recursivamente el sucesor In-Order
                root->right = deleteNode(root->right, temp->key);
            }
        }

        // Si el árbol tenía un solo nodo y fue eliminado
        if (root == nullptr)
            return root;

        // 2. Actualizar la altura del nodo actual
        root->height = 1 + std::max(height(root->left), height(root->right));

        // 3. Obtener el factor de balance del nodo actual
        int balance = balanceFactor(root);

        // 4. Rebalancear el nodo si se encuentra desbalanceado:

        // Caso Izquierda-Izquierda ( LL)
        if (balance > 1 && balanceFactor(root->left) >= 0)
            return rightRotate(root);

        // Caso Izquierda-Derecha (LR)
        if (balance > 1 && balanceFactor(root->left) < 0) {
            root->left = leftRotate(root->left);
            return rightRotate(root);
        }

        // Caso Derecha-Derecha (RR)
        if (balance < -1 && balanceFactor(root->right) <= 0)
            return leftRotate(root);

        // Caso Derecha-Izquierda (RL)
        if (balance < -1 && balanceFactor(root->right) > 0) {
            root->right = rightRotate(root->right);
            return leftRotate(root);
        }

        return root;
    }

    // Función auxiliar para buscar una clave en el subárbol.
    bool search(AVLNode<T>* root, T key)
    {
        if (root == nullptr)
            return false;
        if (root->key == key)
            return true;
        if (key < root->key)
            return search(root->left, key);
        return search(root->right, key);
    }

    /**
     * Función auxiliar para liberar la memoria reservada en el Heap.
     */
    void destroyTree(AVLNode<T>* node) {
        if (node != nullptr) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }

public:
    // Constructor
    AVLTree() : root(nullptr), node_count(0) {}
    
    // Destructor
    ~AVLTree() {
        destroyTree(root);}

    /**
     * Inserción de una clave.
     */
    void insert(T key) { 
        bool inserted = false;
        root = insert(root, key, inserted); 
        if (inserted) node_count++;
    }

    /**
     * Eliminación de una clave.
     */
    void remove(T key) { 
        bool deleted = false;
        root = deleteNode(root, key, deleted); 
        if (deleted) node_count--;
    }

    /**
     * Busqueda de una clave en el árbol.
     */
    bool search(T key) { return search(root, key); }

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

#endif