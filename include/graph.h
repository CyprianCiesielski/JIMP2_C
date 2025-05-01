#ifndef GRAPH_H
#define GRAPH_H

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

#endif