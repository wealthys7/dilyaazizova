#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include "lab3/list/generic.h"
#include "lab4/hash_table/generic.h"
#include "lab3/vector/generic.h"

// Узел

typedef struct GNode {
    long long node_id;
    double lat; // широта
    double lon; // долгота
    int index; // номер узла
    GenericList* adj_edges; // исходящие ребра
} GNode;

// Ребро
typedef struct GEdge {
    long long node_id_from; // откуда
    long long node_id_to; // куда
    double length;
} GEdge;

typedef struct GGraph {
    HashTable *nodes_table;  // Для быстрого поиска узла по ID
    Vector *nodes_list; // Для обращения к узлам по порядковому номеру
} GGraph;

GGraph* create_graph();
void add_node(GGraph *graph, long long id, double lat, double lon);
void add_edge(GGraph *graph, long long from, long long to, double length);
void free_graph(GGraph *graph);
// Читаем данные 
void read_nodes(GGraph *graph, char* path_nodes);
void read_edges(GGraph *graph, char* path_edges);
// Находит ID ближайшего узла в графе по заданным координатам
long long find_closest_node(GGraph *graph, double target_lat, double target_lon);
// Алгоритм Дейкстры
void find_path(GGraph *graph, long long start_id, long long end_id, const char *output_file);

int HashLL(const void *key);
int CmpLL(const void *a, const void *b);
int EqualsLL(void *data, void *value);

#endif
