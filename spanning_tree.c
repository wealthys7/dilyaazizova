#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../lab12/graph.h"
#include "disjoint_set.h"
#include "lab3/list/generic.h"

#define EARTH_RADIUS_M 6371000.0
#define PI_CONST 3.14159265358979323846

typedef struct NodeWithDistance {
    GNode *node;
    double distance; // квадрат расстояния с поправкой на широту (только для сортировки)
} NodeWithDistance;

// Компаратор для сортировки рёбер по длине
static int compare_edge_ptrs(const void *a, const void *b) {
    const GEdge *e1 = *(const GEdge **)a;
    const GEdge *e2 = *(const GEdge **)b;

    if (e1->length < e2->length) {
        return -1;
    }
    if (e1->length > e2->length) {
        return 1;
    }
    return 0;
}

// Компаратор для сортировки соседей по расстоянию
static int compare_node_distances(const void *a, const void *b) {
    const NodeWithDistance *n1 = (const NodeWithDistance *)a;
    const NodeWithDistance *n2 = (const NodeWithDistance *)b;
    if (n1->distance < n2->distance) {
        return -1;
    }
    if (n1->distance > n2->distance) {
        return 1;
    }
    return 0;
}

// Реальное расстояние по поверхности Земли в метрах (формула гаверсинуса)
static double haversine_m(double lat1, double lon1, double lat2, double lon2) {
    double phi1 = lat1 * PI_CONST / 180.0;
    double phi2 = lat2 * PI_CONST / 180.0;
    double dphi = phi2 - phi1;
    double dlambda = (lon2 - lon1) * PI_CONST / 180.0;

    double a = sin(dphi / 2) * sin(dphi / 2)
             + cos(phi1) * cos(phi2) * sin(dlambda / 2) * sin(dlambda / 2);
    if (a > 1.0) {
        a = 1.0;
    }
    return 2.0 * EARTH_RADIUS_M * atan2(sqrt(a), sqrt(1.0 - a));
}

// Квадрат расстояния для выбора соседей.
// Долгота умножается на cos(широты), иначе градусы долготы и широты весят по-разному
static double scaled_dist2(double lat1, double lon1, double lat2, double lon2) {
    double dx = lat1 - lat2;
    double dy = (lon1 - lon2) * cos(lat1 * PI_CONST / 180.0);
    return dx * dx + dy * dy;
}

// Подсчет всех направленных рёбер в списках смежности графа
static size_t count_all_edges(GGraph *graph) {
    size_t count = 0;

    if (graph != NULL && graph->nodes_list != NULL) {
        for (size_t i = 0; i < graph->nodes_list->size; i++) {
            GNode **node_ptr = (GNode **)getVectorItem(graph->nodes_list, i);
            if (node_ptr != NULL && *node_ptr != NULL) {
                GNode *node = *node_ptr;
                if (node->adj_edges != NULL) {
                    Node *cur = node->adj_edges->head;
                    while (cur != NULL) {
                        count++;
                        cur = cur->next;
                    }
                }
            }
        }
    }
    return count;
}

// Построение k ближайших рёбер для каждого здания
void build_edges(GGraph *graph, unsigned char max_edges) {
    if (graph == NULL || graph->nodes_list == NULL) {
        printf("Некорректные входные данные.\n");
        return;
    }
    size_t n = graph->nodes_list->size;
    if (n <= 1) {
        printf("Недостаточно узлов для создания рёбер.\n");
        return;
    }

    NodeWithDistance *distances = malloc(n * sizeof(NodeWithDistance));
    if (distances == NULL) {
        printf("Не выделилась память под массив расстояний.\n");
        return;
    }

    for (size_t i = 0; i < n; i++) {
        GNode *curr_building = *(GNode **)getVectorItem(graph->nodes_list, i);
        if (curr_building == NULL) {
            continue;
        }

        size_t count = 0;
        for (size_t j = 0; j < n; j++) {
            if (i == j) {
                continue;
            }
            GNode *neighbor = *(GNode **)getVectorItem(graph->nodes_list, j);
            if (neighbor != NULL) {
                distances[count].node = neighbor;
                distances[count].distance = scaled_dist2(curr_building->lat, curr_building->lon,
                                                         neighbor->lat, neighbor->lon);
                count++;
            }
        }

        qsort(distances, count, sizeof(NodeWithDistance), compare_node_distances);

        size_t edges_to_add = (count < max_edges) ? count : max_edges;
        for (size_t k = 0; k < edges_to_add; k++) {
            GNode *nb = distances[k].node;
            double real_len = haversine_m(curr_building->lat, curr_building->lon, nb->lat, nb->lon);
            add_edge(graph, curr_building->node_id, nb->node_id, real_len);
        }
    }
    free(distances);
    printf("Рёбра успешно созданы!\n");
}

// Сбор всех рёбер графа.
// Рёбра берём в обоих направлениях: дубликаты Краскал отфильтрует сам (через DSU)
static size_t collect_edges(GGraph *graph, GEdge **edges) {
    size_t idx = 0;

    if (graph == NULL || graph->nodes_list == NULL || edges == NULL) {
        printf("Некорректные входные данные.\n");
        return 0;
    }

    for (size_t i = 0; i < graph->nodes_list->size; i++) {
        GNode **node_ptr = (GNode **)getVectorItem(graph->nodes_list, i);
        if (node_ptr != NULL && *node_ptr != NULL) {
            GNode *node = *node_ptr;
            Node *cur = (node->adj_edges != NULL) ? node->adj_edges->head : NULL;

            while (cur != NULL) {
                GEdge *current_edge = (GEdge *)cur->data;
                if (current_edge != NULL) {
                    edges[idx] = current_edge;
                    idx++;
                }
                cur = cur->next;
            }
        }
    }
    return idx;
}

// Алгоритм Краскала для построения минимального остовного дерева
Vector *kruskal_mst(GGraph *graph) {
    if (graph == NULL || graph->nodes_list == NULL) {
        printf("Некорректные входные данные.\n");
        return NULL;
    }

    size_t total_edges = count_all_edges(graph);
    if (total_edges == 0) {
        printf("В графе нет рёбер.\n");
        return NULL;
    }
    GEdge **edges = calloc(total_edges, sizeof(GEdge *));
    if (edges == NULL) {
        printf("Не выделилась память под массив рёбер.\n");
        return NULL;
    }

    size_t unique_edges = collect_edges(graph, edges);
    qsort(edges, unique_edges, sizeof(GEdge *), compare_edge_ptrs);

    DisjointSet *dsu = dsu_create();
    if (dsu == NULL) {
        printf("Не удалось создать систему непересекающихся множеств.\n");
        free(edges);
        return NULL;
    }

    for (size_t i = 0; i < graph->nodes_list->size; i++) {
        GNode **node_ptr = (GNode **)getVectorItem(graph->nodes_list, i);
        if (node_ptr != NULL && *node_ptr != NULL) {
            GNode *node = *node_ptr;
            long id = node->node_id;
            DSNode *dsu_node = make_node(id);

            if (dsu_node != NULL) {
                setItemHashTable(dsu->trees, &id, &dsu_node, HashLong, compareLong);
            } else {
                printf("Не удалось создать узел DSU.\n");
                freeDisjointSet(dsu);
                free(edges);
                return NULL;
            }
        }
    }

    Vector *mst = createVector(sizeof(GEdge *));
    if (mst == NULL) {
        printf("Не выделилась память под MST.\n");
        freeDisjointSet(dsu);
        free(edges);
        return NULL;
    }

    for (size_t i = 0; i < unique_edges; i++) {
        GEdge *current_edge = edges[i];
        if (current_edge != NULL) {
            DSNode *u = find_set_from_key(dsu, current_edge->node_id_from);
            DSNode *v = find_set_from_key(dsu, current_edge->node_id_to);

            if (u != NULL && v != NULL && find_set(u) != find_set(v)) {
                appendVectorItem(mst, &current_edge);
                union_sets(u, v);
            }
        }
    }
    freeDisjointSet(dsu);
    free(edges);
    return mst;
}

void write_edges_to_file(Vector *edges, char *path_out) {
    if (edges == NULL || path_out == NULL) {
        printf("Некорректные входные данные.\n");
        return;
    }

    FILE *file = fopen(path_out, "w");
    if (file == NULL) {
        printf("Не удалось открыть файл для записи.\n");
        return;
    }

    for (size_t i = 0; i < edges->size; i++) {
        GEdge **edge_ptr = (GEdge **)getVectorItem(edges, i);
        if (edge_ptr != NULL && *edge_ptr != NULL) {
            GEdge *e = *edge_ptr;
            fprintf(file, "%lld\t%lld\t%.10f\n", e->node_id_from, e->node_id_to, e->length);
        }
    }

    fclose(file);
}

int main() {
    GGraph *graph = create_graph();
    if (graph == NULL) {
        printf("Не удалось создать граф.\n");
        return 1;
    }

    read_nodes(graph, "data/buildings.csv");
    printf("Зданий прочитано: %zu\n", graph->nodes_list->size);

    printf("Создание связей между домами\n");
    build_edges(graph, 3);

    printf("Запуск алгоритма Краскала\n");
    Vector *mst = kruskal_mst(graph);
    if (mst == NULL) {
        printf("Не удалось построить остовное дерево.\n");
        free_graph(graph);
        return 1;
    }
    write_edges_to_file(mst, "data/spanning_tree.csv");

    double total_length = 0.0;
    for (size_t i = 0; i < mst->size; i++) {
        GEdge **e = (GEdge **)getVectorItem(mst, i);
        total_length += (*e)->length;
    }
    printf("Рёбер в MST: %zu\n", mst->size);
    printf("Суммарная длина: %.2f м (%.3f км)\n", total_length, total_length / 1000.0);

    vectorFree(mst);
    free_graph(graph);
    printf("Успешно завершено!\n");
    return 0;
}