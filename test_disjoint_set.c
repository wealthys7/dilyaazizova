#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../wealthys7_lab4/disjoint_set.h"

int main() {
    printf("Запуск тестов DSU\n");

    DSNode *node1 = make_node(100);
    assert(node1 != NULL);
    assert(node1->value == 100);
    assert(node1->parent == node1);
    assert(node1->rank == 0);
    printf("make_node: успешно\n");

    assert(find_set(node1) == node1);
    printf("find_set (одиночный узел): успешно\n");

    DisjointSet *dsu = dsu_create();
    assert(dsu != NULL);
    assert(dsu->trees != NULL);
    printf("dsu_create: успешно\n");

    assert(find_set(NULL) == NULL);
    assert(find_set_from_key(NULL, 10) == NULL);
    assert(dsu_create_from_hash_table(NULL) == NULL);
    DSNode *tmp = make_node(1);
    union_sets(NULL, tmp);
    union_sets(tmp, NULL);
    assert(tmp != NULL);
    free(tmp);
    printf("Тесты на NULL: успешно\n");

    DSNode *nodeA = make_node(1);
    DSNode *nodeB = make_node(2);
    union_sets(nodeA, nodeB);
    assert(find_set(nodeA) == find_set(nodeB));
    union_sets(nodeA, nodeB); 
    assert(find_set(nodeA) == find_set(nodeB));
    free(nodeA);
    free(nodeB);
    printf("Идемпотентность: успешно\n");

    HashTable *mock_nodes = createHashTable(sizeof(long), sizeof(int));
    long k1 = 10, k2 = 20;
    int dummy1 = 1, dummy2 = 2;

    setItemHashTable(mock_nodes, &k1, &dummy1, HashLong, compareLong);
    setItemHashTable(mock_nodes, &k2, &dummy2, HashLong, compareLong);

    DisjointSet *dsu_from_ht = dsu_create_from_hash_table(mock_nodes);
    assert(dsu_from_ht != NULL);
    assert(find_set_from_key(dsu_from_ht, 10) != NULL);
    assert(find_set_from_key(dsu_from_ht, 20) != NULL);
    assert(find_set_from_key(dsu_from_ht, 30) == NULL); 
    printf("dsu_create_from_hash_table: успешно\n");

    DSNode *node2 = make_node(200);
    union_sets(node1, node2);
    assert(find_set(node1) == find_set(node2));
    printf("union_sets: успешно\n");

    DSNode *node3 = make_node(300);
    union_sets(node2, node3);
    assert(find_set(node1) == find_set(node3));
    printf("Транзитивность: успешно\n");

    free(node1);
    free(node2);
    free(node3);
    freeDisjointSet(dsu);
    
    freeDisjointSet(dsu_from_ht);
    vectorFree(mock_nodes->values);
    free(mock_nodes);

    printf("\nВсе тесты пройдены успешно!\n");
    return 0;
}