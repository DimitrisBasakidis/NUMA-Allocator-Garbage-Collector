#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "../garbage-collector/cppGarbageCollector.h"

struct Client : public Traceable {
    int id;
    std::string name;

    Client(int i, const std::string& n) : id(i), name(n) {}
};

struct Node : public Traceable {
    Client* client;
    Node* next;
    Node* prev;

    Node(Client* c) : client(c), next(nullptr), prev(nullptr) {}
};

struct List : public Traceable {
    Node* head = nullptr;

    void insert(Client* c) {
        Node* node = new Node(c);
        node->next = head;
        if (head) head->prev = node;
        head = node;
    }

    void clear() {
        head = nullptr;  // Let GC collect everything
    }
};

struct HashTable : public Traceable {
    static const int SIZE = 10;
    List* buckets[SIZE];

    HashTable() {
        for (int i = 0; i < SIZE; ++i)
            buckets[i] = new List();
    }

    void insert(Client* c) {
        int idx = c->id % SIZE;
        buckets[idx]->insert(c);
    }

    void clear() {
        for (int i = 0; i < SIZE; ++i)
            buckets[i]->clear();
    }
};

int main() {
    gcInit(1024 * 1024 * 100);  // 100 MB heap
    std::mt19937 rng(time(nullptr));
    std::uniform_int_distribution<int> idGen(1000, 9999);

    HashTable* table = new HashTable();

    std::vector<Client*> tempRefs;

    for (int i = 0; i < 100; ++i) {
        int id = idGen(rng);
        Client* client = new Client(id, "Client_" + std::to_string(id));
        table->insert(client);

        if (i % 20 == 0)
            tempRefs.push_back(client);  // Hold a few references
    }

    std::cout << "[APP] Clearing table and releasing client refs...\n";
    table->clear();
    table = nullptr;
    tempRefs.clear();  // Now all clients are unreachable

    gc();

    std::cout << "[APP] GC triggered.\n";
    gcFree();

    return 0;
}


