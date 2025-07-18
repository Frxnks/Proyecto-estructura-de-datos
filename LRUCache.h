#ifndef LRUCACHE_H
#define LRUCACHE_H
#include <string>
#include <vector>

// Nodo para el cache LRU
class CacheNode {
public:
    std::string key;
    std::string value;
    CacheNode* next;
    
    CacheNode(std::string _key, std::string _value) : key(_key), value(_value), next(nullptr) {}
};

// Stack para implementar LRU
class LRUStack {
private:
    CacheNode* top;
    int size;
    int maxSize;
    
public:
    // Constructor al que se le pasa el tamano maximo del stack.
    LRUStack(int _maxSize) : top(nullptr), size(0), maxSize(_maxSize) {}
    
    // Destructor para liberar memoria y evitar el memory leak.
    ~LRUStack() {
        while (top != nullptr) {
            CacheNode* temp = top;
            top = top->next;
            delete temp;
        }
    }
    
    void push(std::string key, std::string value) {
        CacheNode* newNode = new CacheNode(key, value);
        newNode->next = top;
        top = newNode;
        size++;
        
        // Si excede el tamano, eliminar el ultimo
        if (size > maxSize) {
            removeLast();
        }
    }
    
    bool remove(std::string key) {
        if (top == nullptr) return false;
        
        // Si es el primero
        if (top->key == key) {
            CacheNode* temp = top;
            top = top->next;
            delete temp;
            size--;
            return true;
        }
        
        // Buscar en el resto
        CacheNode* current = top;
        while (current->next != nullptr) {
            if (current->next->key == key) {
                CacheNode* temp = current->next;
                current->next = current->next->next;
                delete temp;
                size--;
                return true;
            }
            current = current->next;
        }
        return false;
    }
    
    void removeLast() {
        if (top == nullptr) return;
        
        if (top->next == nullptr) {
            delete top;
            top = nullptr;
            size = 0;
            return;
        }
        
        CacheNode* current = top;
        while (current->next->next != nullptr) {
            current = current->next;
        }
        delete current->next;
        current->next = nullptr;
        size--;
    }
    
    std::string getLRUKey() {
        if (top == nullptr) return "";
        
        CacheNode* current = top;
        while (current->next != nullptr) {
            current = current->next;
        }
        return current->key;
    }
    
    // Obtener todas las claves en orden de uso (mas reciente primero)
    std::vector<std::string> getAllKeys() const {
        std::vector<std::string> keys;
        CacheNode* current = top;
        while (current != nullptr) {
            keys.push_back(current->key);
            current = current->next;
        }
        return keys;
    }
    
    int getSize() const { return size; }
    int getMaxSize() const { return maxSize; }
    bool isEmpty() const { return size == 0; }
    bool isFull() const { return size >= maxSize; }
};

// Tabla Hash para el cache
class HashTable {
private:
    static const int TABLE_SIZE = 101; // Numero primo para mejor distribucion
    CacheNode* table[TABLE_SIZE];
    
    // distribucion de claves a indices
    int hashFunction(const std::string& key) {
        int hash = 0;
        for (char c : key) {
            hash = (hash * 31 + c) % TABLE_SIZE;
        }
        return hash;
    }
    
public:
    HashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            table[i] = nullptr;
        }
    }
    
    ~HashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            CacheNode* current = table[i];
            while (current != nullptr) {
                CacheNode* temp = current;
                current = current->next;
                delete temp;
            }
        }
    }
    
    void put(std::string key, std::string value) {
        int index = hashFunction(key);
        
        // Buscar si ya existe
        CacheNode* current = table[index];
        while (current != nullptr) {
            if (current->key == key) {
                current->value = value; // Actualizar valor
                return;
            }
            current = current->next;
        }
        
        // Insertar nuevo nodo
        CacheNode* newNode = new CacheNode(key, value);
        newNode->next = table[index];
        table[index] = newNode;
    }
    
    std::string get(std::string key) {
        int index = hashFunction(key);
        CacheNode* current = table[index];
        
        while (current != nullptr) {
            if (current->key == key) {
                return current->value;
            }
            current = current->next;
        }
        return ""; // No encontrado
    }
    
    bool remove(std::string key) {
        int index = hashFunction(key);
        CacheNode* current = table[index];
        
        if (current == nullptr) return false;
        
        // Si es el primero
        if (current->key == key) {
            table[index] = current->next;
            delete current;
            return true;
        }
        
        // Buscar en el resto
        while (current->next != nullptr) {
            if (current->next->key == key) {
                CacheNode* temp = current->next;
                current->next = current->next->next;
                delete temp;
                return true;
            }
            current = current->next;
        }
        return false;
    }
    
    bool contains(std::string key) {
        return get(key) != "";
    }
};

// Cache LRU combinando HashTable y Stack
class LRUCache {
private:
    HashTable* hashTable;
    LRUStack* lruStack;
    int maxSize;
    
public:
    LRUCache(int _maxSize) : maxSize(_maxSize) {
        hashTable = new HashTable();
        lruStack = new LRUStack(_maxSize);
    }
    
    ~LRUCache() {
        delete hashTable;
        delete lruStack;
    }
    
    std::string get(std::string key) {
        std::string value = hashTable->get(key);
        if (value != "") {
            // Mover al tope (mas reciente)
            lruStack->remove(key);
            lruStack->push(key, value);
            return value;
        }
        return ""; // No encontrado
    }
    
    void put(std::string key, std::string value) {
        // Si ya existe, actualizar
        if (hashTable->contains(key)) {
            hashTable->put(key, value);
            lruStack->remove(key);
            lruStack->push(key, value);
            return;
        }
        
        // Si esta lleno, eliminar LRU
        if (lruStack->isFull()) {
            std::string lruKey = lruStack->getLRUKey();
            hashTable->remove(lruKey);
            lruStack->removeLast();
        }
        
        // Insertar nuevo
        hashTable->put(key, value);
        lruStack->push(key, value);
    }
    
    bool contains(std::string key) {
        return hashTable->contains(key);
    }
    
    int getSize() const {
        return lruStack->getSize();
    }
    
    int getMaxSize() const {
        return maxSize;
    }
    
    // Obtener todas las claves del cache en orden de uso (mas reciente primero)
    std::vector<std::string> getAllKeys() const {
        return lruStack->getAllKeys();
    }
    
    // Limpiar todo el contenido del cache
    void clear() {
        // Limpiar la tabla hash
        delete hashTable;
        hashTable = new HashTable();
        
        // Limpiar el stack LRU
        delete lruStack;
        lruStack = new LRUStack(maxSize);
    }
};

#endif
