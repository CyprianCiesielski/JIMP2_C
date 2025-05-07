#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include "graph.h"
#include "partition.h"

void print_statistics(const Graph *graph, const Partition_data *partition_data, int parts, float accuracy, int max_edges_cut, int precompute, double execution_time);

#endif