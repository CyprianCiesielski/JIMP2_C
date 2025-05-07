#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct Partition_data;
typedef struct Partition_data Partition_data;

typedef struct Node
{
    int vertex;
    int *neighbors;
    int neighbor_count;
    int part_id;
    int neighbor_capacity;
} Node;

typedef struct Graph
{
    int vertices;
    int edges;
    int parts;
    int min_count;
    int max_count;
    Node *nodes;
} Graph;

void print_part_neighbors(int **neighbors, int size);
void inicialize_graph(Graph *graph, int vertices);
void count_edges(Graph *graph);
void assign_min_max_count(Graph *graph, int parts, float accuracy);
void assing_parts(Graph *graph, int parts);
void free_graph(Graph *graph);

#endif