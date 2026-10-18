#include "lab4/hash_table/generic.h" 
// Узел
typedef struct DSNode {
    long value; // Идентификатор узла
    struct DSNode *parent; // Указатель на родительский узел
    int rank; // Ранг узла 
} DSNode;

// Система непересекающихся множеств
typedef struct {
    HashTable *trees;  // Хэш-таблица для хранения корневых узлов множеств
} DisjointSet;

// Здание
typedef struct {
    long id; // Идентификатор здания
    double lat; // Широта
    double lon; // Долгота
} Building;

// Ребро
typedef struct {
    long u; // Идентификатор начального узла
    long v; // Идентификатор конечного узла
    double weight; // Вес рёбра 
} Edge;

DSNode *make_node(long value);
DisjointSet *dsu_create();
DisjointSet *dsu_create_from_hash_table(HashTable *nodes);
DSNode *find_set(DSNode *node);
DSNode *find_set_from_key(DisjointSet *dsu, long key);
void union_sets(DSNode *a, DSNode *b);
void freeDisjointSet(DisjointSet *dsu);
int HashLong(const void *key);
int compareLong(const void *a, const void *b);
