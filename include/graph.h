#ifndef GRAPH_H
#define GRAPH_H

typedef struct Node {
    int vertex;                
    int *neighbors;             
    int neighbor_count;        
    int neighbor_capacity;      
} Node;

typedef struct Graph {
    int vertices;               
    Node *nodes;               
} Graph;

#endif