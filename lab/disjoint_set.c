#include "disjoint_set.h"
#include <stdlib.h>
#include <string.h>

// Компаратор для long
int compareLong(const void *a, const void *b) {
    long val_a = *(const long *)a;
    long val_b = *(const long *)b;
    if (val_a == val_b) return 0;
    return (val_a < val_b) ? -1 : 1;
}

int HashLong(const void *key) {
    long val = *(const long *)key;
    return (int)val;
}

DisjointSet *dsu_create() {
    DisjointSet *dsu = malloc(sizeof(DisjointSet));
    if (dsu == NULL) {
        printf("Не выделилась память под DSU.\n");
        return NULL;
    }

    dsu->trees = createHashTable(sizeof(long), sizeof(DSNode*));
    if (dsu->trees == NULL) {
        printf("Не выделилась память под хэш-таблицу DSU.\n");
        free(dsu);
        return NULL;
    }

    return dsu;
}

// Создание узла
DSNode *make_node(long value) {
    DSNode *node = malloc(sizeof(DSNode));
    if (node == NULL) {
        printf("Не выделилась память под узел DSU.\n");
        return NULL;
    }

    node->value = value;
    node->parent = node; // Изначально узел сам себе папа
    node->rank = 0;
    
    return node;
}

// Создание DSU из HashTable
DisjointSet *dsu_create_from_hash_table(HashTable *nodes) {
    if (nodes == NULL) {
        printf("Хэш-таблица пустая.\n");
        return NULL;
    }

    DisjointSet *dsu = dsu_create();
    if (dsu == NULL) {
        printf("Не выделилась память под DSU.\n");
        return NULL;
    }

    for (size_t i = 0; i < nodes->capacity; i++) {
        char *slot = getVectorItem(nodes->values, i);
        // Проверяем, что слот занят и содержит ключ
        if (slot && slot[0] == SLOT_OCCUPIED) {
            long key; // Извлекаем ключ из слота
            memcpy(&key, slot + 1, sizeof(long)); // лежит после байта статуса
            
            DSNode *node = make_node(key);
            if (node == NULL) {
                printf("Не выделилась память под узел DSU.\n");
                freeDisjointSet(dsu);
                return NULL;
            }
            // Добавляем узел в DSU
            setItemHashTable(dsu->trees, &key, &node, HashLong, compareLong);
        }
    }
    return dsu;
}

// Поиск корневого узла множества с оптимизацией сжатия пути
DSNode *find_set(DSNode *node) {
    if (node == NULL) {
        printf("Некорректные входные данные.\n");
        return NULL;
    }

    if (node->parent != node) {
        // Рекурсивно сжимаем путь, папа становится корнем
        node->parent = find_set(node->parent);
    }
    return node->parent;
}

// Поиск корневого узла множества по ключу
DSNode *find_set_from_key(DisjointSet *dsu, long key) {
    if (dsu == NULL) {
        printf("Некорректные входные данные.\n");
        return NULL;
    }

    // Получаем указатель на узел по ключу из хэш-таблицы
    DSNode **node_ptr = getItemHashTable(dsu->trees, &key, HashLong, compareLong);
    if (node_ptr == NULL) {
        printf("Узел с ключом %ld не найден.\n", key);
        return NULL;
    }

    return find_set(*node_ptr);
}

// Объединение множеств по рангу
void union_sets(DSNode *a, DSNode *b) {
    if (a == NULL || b == NULL) {
        printf("Некорректные входные данные.\n");
        return;
    }

    // Находим корни множеств
    DSNode *rootA = find_set(a);
    DSNode *rootB = find_set(b);

    if (rootA == rootB) {
        printf("Узлы уже находятся в одном множестве.\n");
        return;
    }

    // Объединяем по рангу
    if (rootA->rank < rootB->rank) {
        rootA->parent = rootB;
    } else if (rootA->rank > rootB->rank) {
        rootB->parent = rootA;
    } else {
        rootB->parent = rootA;
        rootA->rank++;
    }
}

// Освобождение памяти
void freeDisjointSet(DisjointSet *dsu) {
    if (dsu == NULL) return;

    // Освобождаем память для всех узлов в хэш-таблице
    for (size_t i = 0; i < dsu->trees->capacity; i++) {
        char *slot = getVectorItem(dsu->trees->values, i);
        if (slot && slot[0] == SLOT_OCCUPIED) {
            DSNode *node;
            memcpy(&node, slot + 1 + sizeof(long), sizeof(DSNode*));
            free(node);
        }
    }

    vectorFree(dsu->trees->values);
    free(dsu->trees);
    free(dsu);
}
