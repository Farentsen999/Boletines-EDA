#include <iostream>
#include <list>
#include <algorithm>

// Estructura que representa un nodo de un Binomial min-Heap
struct Node
{
    int data;   // Clave almacenado en el nodo
    int degree; // Gradoo del ábol (nmero de hijos)
    Node *child, *sibling, *parent; // Punteros a nodo hijo, hermano y padre
};

class binomial_heap
{
    protected:
        std::list<Node *> roots; // Lista de raíces que forman el bosque del heap

    private:

        // Libera de forma la memoria del árbol
        void destroyTree(Node* n) {
            while (n) {
                destroyTree(n->child);
                Node* next = n->sibling;
                delete n;
                n = next;
            }
        }

        // Crea e inicializa un nuevo nodo
        Node* newNode(int key)
        {
            Node *temp = new Node;
            temp->data = key;
            temp->degree = 0;
            temp->child = temp->parent = temp->sibling = NULL;
            return temp;
        }

        // Une dos Binomial Heaps del mismo grado
        Node* mergeBinomialTrees(Node *b1, Node *b2)
        {
            // Garantizar que b1 contenga la clave menor para preservar la propiedad de heap
            if (b1->data > b2->data)
                std::swap(b1, b2);

            // Hacer que b2 sea el nuevo hijo de b1
            b2->parent = b1;
            b2->sibling = b1->child; // El antiguo primer hijo de b1 pasa a ser hermano de b2
            b1->child = b2;          // b2 se convierte en el nuevo primer hijo de b1
            b1->degree++;            // Incrementa el grado de la raíz b1

            return b1;
        }

        // Entrelaza dos listas de Binomial Heaps.
        // Ordena las raíces por sus grados en forma no decreciente.
        std::list<Node*> unionBionomialHeap(std::list<Node*> l1, std::list<Node*> l2)
        {
            std::list<Node*> _new;
            std::list<Node*>::iterator it = l1.begin();
            std::list<Node*>::iterator ot = l2.begin();

            // Se comparan los grados de las raíces y se agregan en orden ascendente
            while (it != l1.end() && ot != l2.end())
            {
                if ((*it)->degree <= (*ot)->degree)
                {
                    _new.push_back(*it);
                    it++;
                }
                else
                {
                    _new.push_back(*ot);
                    ot++;
                }
            }

            // Se inserta los elementos restantes de l1
            while (it != l1.end())
            {
                _new.push_back(*it);
                it++;
            }

            // Se inserta los elementos restantes de l2
            while (ot != l2.end())
            {
                _new.push_back(*ot);
                ot++;
            }

            return _new;
        }

        // Ajusta el heap fusionando aquellos Binomial Heaps que tengan el mismo grado.
        std::list<Node*> adjust(std::list<Node*> _heap)
        {
            if (_heap.size() <= 1) // Si hay 0 o 1 árboles, no se requiere ajuste
                return _heap;

            std::list<Node*> new_heap;
            std::list<Node*>::iterator it1, it2, it3;
            it1 = it2 = it3 = _heap.begin();

            // Posicionar iteradores contiguos: it2 = it1 + 1, it3 = it1 + 2
            if (_heap.size() == 2)
            {
                it2++;
                it3 = _heap.end();
            }
            else
            {
                it2++;
                it3 = it2;
                it3++;
            }

            while (it1 != _heap.end())
            {
                // Si solo queda un elemento por procesar en la iteración
                if (it2 == _heap.end())
                {
                    it1++;
                }
                // Caso 1: Los grados son distintos, solo avanzamos los punteros
                else if ((*it1)->degree < (*it2)->degree)
                {
                    it1++;
                    it2++;
                    if (it3 != _heap.end())
                        it3++;
                }
                // Caso 2: Hay tres árboles consecutivos con el mismo grado.
                // Se avanza para permitir que la combinación ocurra en los dos últimos.
                else if (it3 != _heap.end() &&
                        (*it1)->degree == (*it2)->degree &&
                        (*it1)->degree == (*it3)->degree)
                {
                    it1++;
                    it2++;
                    it3++;
                }
                // Caso 3: Dos árboles consecutivos tienen el mismo grado. Se deben unir.
                else if ((*it1)->degree == (*it2)->degree)
                {
                    *it1 = mergeBinomialTrees(*it1, *it2); // Unir ambos árboles
                    it2 = _heap.erase(it2);                // Eliminar la raíz duplicada de la std::lista
                    if (it3 != _heap.end())
                        it3++;
                }
            }

            return _heap;
        }

        // Se inserta un único Binomial Heap dentro de un Binomial Heap existente
        std::list<Node*> insertATreeInHeap(std::list<Node*> _heap, Node *tree)
        {
            std::list<Node*> temp;
            temp.push_back(tree); // Se cra un heap temporal con el árbol único

            // Se fusiona y ajustan las estructuras
            temp = unionBionomialHeap(_heap, temp);
            return adjust(temp);
        }

        // Se extrae la raíz de un Binomial Heap y se revierte la lista de sus hijos.
        // Se onvierten sus subárboles descendientes en un nuevo Binomial Heap válido.
        std::list<Node*> removeMinFromTreeReturnBHeap(Node *tree)
        {
            std::list<Node*> heap;
            Node *temp = tree->child;
            Node *lo;

            // Se ecorren los hijos de la raíz eliminada
            while (temp)
            {
                lo = temp;
                temp = temp->sibling;
                lo->sibling = NULL;    // Se desvinculan de sus hermanos
                heap.push_front(lo);   // Se inserta al inicio para invertir el orden de los grados
            }
            return heap;
        }

        // Se inserta una nueva clave en el Binomial Heap
        std::list<Node*> insert(std::list<Node*> _head, int key)
        {
            Node *temp = newNode(key);
            return insertATreeInHeap(_head, temp);
        }

        // Se retorna el puntero al nodo que contiene el valor mínimo en todo el heap.
        Node* getMin(std::list<Node*> _heap)
        {
            std::list<Node*>::iterator it = _heap.begin();
            Node *temp = *it;

            while (it != _heap.end())
            {
                if ((*it)->data < temp->data)
                    temp = *it;
                it++;
            }
            return temp;
        }

        // Se eimina el elemento mínimo del heap, reorganizando el resto de árboles
        std::list<Node*> extractMin(std::list<Node*> _heap)
        {
            std::list<Node*> new_heap, lo;
            Node *temp;

            // 1. Se obtiene la raíz con el valor mínimo
            temp = getMin(_heap);

            // 2. Se construye una nueva lista de raíces omitiendo el árbol que contiene el mínimo
            std::list<Node*>::iterator it = _heap.begin();
            while (it != _heap.end())
            {
                if (*it != temp)
                {
                    new_heap.push_back(*it);
                }
                it++;
            }

            // 3. Se obtiene un heap compuesto por los hijos de la raíz eliminada
            lo = removeMinFromTreeReturnBHeap(temp);

            // 4. Se une el heap restante con el heap formado por los hijos del nodo eliminado
            new_heap = unionBionomialHeap(new_heap, lo);
            new_heap = adjust(new_heap);

            delete temp; // Liberar la memoria del nodo extraído
            return new_heap;
        }

        // Funcion auxiliar que imprime de forma los nodos del Binomial Heap en recorrido preorder
        void printTree(Node *h)
        {
            while (h)
            {
                std::cout << h->data << " ";
                printTree(h->child);
                h = h->sibling;
            }
        }

        // Imprime todos los elementos del Heap Binomial recorriendo cada uno de sus árboles
        void printHeap(std::list<Node*> _heap)
        {
            std::list<Node*>::iterator it = _heap.begin();
            while (it != _heap.end())
            {
                printTree(*it);
                it++;
            }
        }

        public:
        /** Constructor para crear un Binomial Heap vacio */
        binomial_heap(void)
        {
        }

        /** Destructor */
        ~binomial_heap() {
            for (Node* r : roots) {
                destroyTree(r);
            }
        }

        /** Constructor para crear un Binomial Heap a partir de un vector */
        binomial_heap(const std::vector<int> &v)
        {
            for (int k : v)
                roots = insert(roots, k);
        }

        /** Retorna la menor de las claves sin removerla */
        int top(void)
        {
            return getMin(roots)->data;
        }

         /** Inserta una nueva clave */
        void insert(int key)
        {
            roots = insert(roots, key);
        }

        /** Remueve la menor de las claves*/
        void pop(void)
        {
            roots = extractMin(roots);
        }

        /** Combina 2 Binomial Heaps */
        void meld(const binomial_heap &h)
        {
            roots = adjust(unionBionomialHeap(roots, h.roots));
        }

         /** Indica si el heap esta o no vacio */
        bool empty(void)
        {
            return roots.empty();
        }

        /**Imprime de forma los nodos del Binomial Heap en recorrido preorder.*/
        void print(void)
        {
            printHeap(roots);
        }
};