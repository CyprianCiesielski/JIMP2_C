#ifndef PARTITION_H
#define PARTITION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations - zastępują #include "graph.h"
struct Graph;
typedef struct Graph Graph;

// Definicje struktur dla partycji
typedef struct Part
{
    int part_id;
    int part_vertex_count;
    int *part_vertexes; // Tablica wierzchołków w tej części
    int capacity;       // Pojemność tablicy wierzchołków
    // inne pola...
} Part;

typedef struct Partition_data
{
    int parts_count;
    Part *parts;
    
} Partition_data;

void initialize_partition_data(Partition_data *partition_data, int parts);
void free_partition_data(Partition_data *partition_data, int parts);
void add_partition_data(Partition_data *partition_data, int part_id, int vertex);
int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size);
// Pozostałe prototypy funkcji...

#endif