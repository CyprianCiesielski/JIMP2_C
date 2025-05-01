#ifndef PARTITON_H
#define PARTITON_H
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Part_data
{
    int capacity;          // pojemność tablicy
    int *part_vertexes;    // wierzchołki w każdej części
    int part_vertex_count; // liczba wierzchołków w każdej części
} Part_data;

typedef struct Partition_data
{
    Part_data *parts; // dane o częściach
} Partition_data;

void initialize_partition_data(Partition_data *partition_data, int parts);

void add_partition_data(Partition_data *partition_data, int part_id, int vertex);

void free_partition_data(Partition_data *partition_data, int parts);

void print_partition_data(const Partition_data *partition_data, int parts);

#endif