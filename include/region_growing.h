#ifndef REGION_GROWING_H
#define REGION_GROWING_H

#include "graph.h"
#include "partition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

struct Queue
{
    int *items;
    int front;
    int rear;
    int max_size;
};

int is_empty(struct Queue *queue);

void add_to_queue(struct Queue *queue, int item);

void remove_from_queue(struct Queue *queue);

int region_growing(Graph *graph, int parts, Partition_data *partition_data, float accuracy);

int *generate_seed_points(Graph *graph, int parts);

void check_partition_connectivity(Graph *graph, int parts);
#endif // REGION_GROWING_H
