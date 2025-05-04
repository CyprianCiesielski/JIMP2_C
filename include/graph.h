#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "fm_optimization.h"

typedef struct Node
{
    int vertex;
    int *neighbors;
    int neighbor_count;
    int part_id;
    int neighbor_capacity;   // pojemność tablicy sąsiadów
    int *gain;               // zyski dla węzła
    BucketNode *bucket_node; // wskaźnik do węzła kubełka
    int bucket_index;        // indeks kubełka
} Node;

typedef struct Graph
{
    int vertices;
    int edges;
    int parts;
    Node *nodes;
} Graph;

void inicialize_graph(Graph *graph, int vertices);
void count_edges(Graph *graph);
void assing_parts(Graph *graph, int parts);
void free_graph(Graph *graph);

#endif