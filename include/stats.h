#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include <math.h>
#include "graph.h"
#include "partition.h"

// funkcja wyswietlajaca rozne statystyki dotyczace grafu i jego podzialu
void print_statistics(const Graph *graph, const Partition_data *partition_data, 
                     int parts, float accuracy, int precompute, double execution_time);

// funkcja wyswietlajaca metryki prekomputacji
void print_precompute_metrics(const Graph *graph, const Partition_data *partition_data, int parts);

#endif
