#include <math.h>
#include "./graph.h"
#include "lab3/comparators.h"
#include "lab3/list/generic.h"
#include "lab4/hash_table/generic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <stdbool.h>

// Хеширование для long long
int HashLL(const void *key) {
    long long k = *(const long long*)key;
    return (int)k;
}

// Сравнение ключей (возвращает 0, если равны)
int CmpLL(const void *a, const void *b) {
    long long la = *(const long long*)a;
    long long lb = *(const long long*)b;
    if (la == lb) return 0;
    return (la > lb) ? 1 : -1;
}

// Сравнение для поиска в списке (возвращает 1, если равны)
int EqualsLL(void *data, void *value) {
    return (*(long long*)data == *(long long*)value) ? 1 : 0;
}

// Создаем
GGraph *create_graph(){
    GGraph *graph = (GGraph*)malloc(sizeof(GGraph));
    if (graph == NULL){
        printf("Некорректные входные данные\n");
        return NULL;
    }

    graph->nodes_table = createHashTable(sizeof(long long), sizeof(GNode*));
    graph->nodes_list = createVector(sizeof(GNode*));

    return graph;
}

// Добавляем узел
void add_node(GGraph *graph, long long id, double lat, double lon){
    if (graph == NULL){
        printf("Некорректные входные данные\n");
        return;
    }

    GNode *new_node =(GNode*)malloc(sizeof(GNode));
    if (new_node == NULL){
        printf("Ошибка выделения памяти\n");
        return;
    }

    new_node->node_id = id;
    new_node->lat = lat;
    new_node->lon = lon;
    new_node->index = (int)graph->nodes_list->size;

    // Список ребер, выход из этого узла
    new_node->adj_edges = createList(sizeof(GEdge));
    // Добавляем в хеш-таблицу
    setItemHashTable(graph->nodes_table, &id, &new_node, HashLL, CmpLL);
    // Добавляем в общий список всех узлов
    appendVectorItem(graph->nodes_list, &new_node);
}

// Добавляем ребро между 2 точками
void add_edge(GGraph *graph, long long from, long long to, double length){
    if (graph == NULL){
        printf("Некорректные входные данные\n");
        return;
    }

    // Ищем узел, из которого выходит ребро
    GNode **from_node = (GNode**)getItemHashTable(graph->nodes_table, &from, HashLL, CmpLL);

    if (from_node && *from_node) {
        GEdge edge; // Создаем временную структуру ребра
        edge.node_id_from = from;
        edge.node_id_to = to;
        edge.length = length;

        // Добавляем ребро в список ребер этого узла
        appendItem((*from_node)->adj_edges, &edge);
    }
}

void free_graph(GGraph *graph) {
    if (!graph) return;

    // Очищаем каждый узел
    for (size_t i = 0; i < graph->nodes_list->size; i++) {
        GNode *node = *(GNode**)getVectorItem(graph->nodes_list, i);
        freeList(node->adj_edges); // Удаляем список ребер этого узла
        free(node); // Удаляем сам узел
    }

    vectorFree(graph->nodes_list);
    freeHashTable(graph->nodes_table);
    free(graph);
}

// Чтение файла с узлами
void read_nodes(GGraph *graph, char* path_nodes) {
    FILE* f = fopen(path_nodes, "r");
    if (f == NULL) {
        printf("Ошибка открытия файла узлов\n");
        return;
    }

    char line[1024];
    // Пропускаем заголовок
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f)) {
        // Убираем символ переноса строки в конце, если он есть
        line[strcspn(line, "\r\n")] = 0;

        char *id_str = strtok(line, "\t");
        char *addr_str = strtok(NULL, "\t");
        char *lat_str = strtok(NULL, "\t");
        char *lon_str = strtok(NULL, "\t");

        // Если адреса нет (два таба подряд), strtok вернет lat_str в третьем вызове,
        // либо нам нужно проверить, что мы смогли считать ID, широту и долготу.
        if (id_str && lat_str && lon_str) {
            long long node_id = strtoll(id_str, NULL, 10);
            double lat = strtod(lat_str, NULL);
            double lon = strtod(lon_str, NULL);
            
            add_node(graph, node_id, lat, lon);
        } else if (id_str && addr_str && lat_str && !lon_str) {
            // Случай, когда адреса не было, и lat/lon сместились на одну позицию влево
            long long node_id = strtoll(id_str, NULL, 10);
            double lat = strtod(addr_str, NULL);
            double lon = strtod(lat_str, NULL);
            
            add_node(graph, node_id, lat, lon);
        } else {
            printf("Ошибка чтения строки для узла графа\n");
        }
    }
    fclose(f);
}
// Чтение файла с ребрами
void read_edges(GGraph *graph, char* path_edges) {
    if (graph == NULL || path_edges == NULL) {
        return;
    }

    FILE* f = fopen(path_edges, "r");
    if (f == NULL) {
        printf("Ошибка открытия файла рёбер\n");
        return;
    }

    char line[1024];
    // Пропускаем заголовок CSV
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return;
    }

    while (fgets(line, sizeof(line), f) != NULL) {
        long long from_id;
        long long to_id;
        double length;

        // Читаем только ID начала, ID конца и длину. 
        if (sscanf(line, "%lld,%lld,%lf", &from_id, &to_id, &length) == 3) {
            add_edge(graph, from_id, to_id, length);
        }
    }

    fclose(f);
}

// Поиск ближайшего перекрестка к заданным координатам
long long find_closest_node(GGraph *graph, double target_lat, double target_lon){
    long long closet_id = -1; // Переменная для хранения ID лучшего результата
    double min_dist = DBL_MAX;

    for(size_t i = 0; i < graph->nodes_list->size; i++) {
        // Достаем узел
        GNode ** node_ptr = (GNode**)getVectorItem(graph->nodes_list, i);
        GNode *node = *node_ptr;

        // Считаем квадрат расстояния
        double d_lat = node->lat - target_lat;
        double d_lon = node->lon - target_lon;
        double dist = d_lat * d_lat + d_lon * d_lon;

        if (dist < min_dist) {
            min_dist = dist;
            closet_id = node->node_id;
        }
    }
    return closet_id; // ID самого близкого узла
}

int* calculate_dijkstra(GGraph *graph, long long start_id, long long end_id){
    size_t n = graph->nodes_list->size; // количество узлов в городе
    double *dist = malloc(n * sizeof(double)); // массив кратч расст
    int *parent_id = malloc(n * sizeof(int)); // массив родителей
    bool *visited = calloc(n, sizeof(bool)); // массив посещенных или не посещенных

    for (size_t i = 0; i < n; i++) {
        dist[i] = DBL_MAX; // Изначально всё «бесконечность»
        visited[i] = false;
        parent_id[i] = -1; // Изначально нет пап
    }

    dist[start_id] = 0; // Путь от старта до самого себя равен нулю

    for (size_t i = 0; i < n; i++) {
        int current_idx = -1;
        for (size_t j = 0; j < n; j++) {
            if (!visited[j] && (current_idx == -1 || dist[j] < dist[current_idx])) {
                current_idx = (int)j;
            }
        }

        // Если до самого близкого узла расстояние бесконечно, то до остальных не дойти
        if (current_idx == -1 || dist[current_idx] == DBL_MAX){
            break;
        }
        // Нашли финиш - выходим
        if (current_idx == end_id){
            break;
        }

        visited[current_idx] = true;
        // Чекаем всех соседей узла u
        GNode *u_node = *(GNode**)getVectorItem(graph->nodes_list, current_idx);
        Node *curr_edge_node = u_node->adj_edges->head;

        while (curr_edge_node) {
            GEdge *edge = (GEdge*)curr_edge_node->data;
            // Находим индекс соседа v
            GNode **v_node_ptr = (GNode**)getItemHashTable(graph->nodes_table, &edge->node_id_to, HashLL, CmpLL);
            
            if (v_node_ptr) {
                int neighbor_idx = (*v_node_ptr)->index;
                // Релаксация: Если путь через U короче, чем известный ранее путь в V
                if (dist[current_idx] + edge->length < dist[neighbor_idx]) { // Нашли путь короче
                    dist[neighbor_idx] = dist[current_idx] + edge->length; // Обновляем расстояние
                    parent_id[neighbor_idx] = current_idx; // Запомнили папу v
                }
            }
            curr_edge_node = curr_edge_node->next;
        }
    }
    if (dist[end_id] == DBL_MAX) {
        free(parent_id);
        parent_id = NULL;
    }

    free(dist);
    free(visited);
    return parent_id;
}
// Функция подготовки и записи (IO-bound)
void find_path(GGraph *graph, long long start_id, long long end_id, const char *output_file) {
    GNode **start_node = getItemHashTable(graph->nodes_table, &start_id, HashLL, CmpLL);
    GNode **end_node = getItemHashTable(graph->nodes_table, &end_id, HashLL, CmpLL);

    if (start_node == NULL || end_node == NULL) {
        FILE *out = fopen(output_file, "w");
        if (out) fclose(out);
        return;
    }

    int start_idx = (*start_node)->index;
    int end_idx = (*end_node)->index;

    // Вызываем CPU-часть
    int *parent_id = calculate_dijkstra(graph, start_idx, end_idx);

    // Выполняем IO-часть (запись)
    FILE *out = fopen(output_file, "w");
    if (out != NULL) {
        if (parent_id != NULL) {
            size_t n = graph->nodes_list->size;
            int *path_stack = malloc(n * sizeof(int));
            int count = 0;
            int curr = end_idx;

            while (curr != -1) {
                path_stack[count++] = curr;
                curr = parent_id[curr];
            }
            for (int i = count - 1; i >= 0; i--) {
                GNode *p = *(GNode**)getVectorItem(graph->nodes_list, path_stack[i]);
                fprintf(out, "%.6f %.6f\n", p->lat, p->lon);
            }
            free(path_stack);
        }
        fclose(out);
    }

    if (parent_id) free(parent_id);
}

/*
#ifndef TESTING
int main(int argc, char *argv[]) {
    if (argc < 4) return 1;

    char nodes_path[512], edges_path[512];
    sprintf(nodes_path, "%s/nodes.csv", argv[1]);
    sprintf(edges_path, "%s/edges.csv", argv[1]);

    GGraph *graph = create_graph();
    read_nodes(graph, nodes_path);
    read_edges(graph, edges_path);

    FILE *in = fopen(argv[2], "r");
    if (!in) { 
        free_graph(graph);
        return 1; 
    }

    double start_lat, start_lon, end_lat, end_lon;
    if (fscanf(in, "%lf %lf %lf %lf", &start_lat, &start_lon, &end_lat, &end_lon) < 4) {
        rewind(in);
        fscanf(in, "%lf %lf", &start_lat, &start_lon);
        fscanf(in, "%lf %lf", &end_lat, &end_lon);
    }
    fclose(in);

    long long start_id = find_closest_node(graph, start_lat, start_lon);
    long long end_id = find_closest_node(graph, end_lat, end_lon);

    find_path(graph, start_id, end_id, argv[3]);
    free_graph(graph);
    return 0;
}
#endif
*/