#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include "partition.h"

typedef struct Node
{
    int vertex;
    int *neighbors;
    int neighbor_count;
    int part_id;
    int neighbor_capacity; // pojemność tablicy sąsiadów
} Node;

typedef struct Graph
{
    int vertices;
    Node *nodes;
} Graph;

int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size);
void print_part_neighbors(int **neighbors, int size);

#endif