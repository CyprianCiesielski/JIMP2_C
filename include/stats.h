#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include "graph.h"
#include "partition.h"

// funkcja wyswietlajaca rozne statystyki dotyczace grafu i jego podzialu:
// - ile jest wierzcholkow i krawedzi
// - na ile czesci jest podzielony i jaka jest dokladnosc podzialu
// - ile wierzcholkow jest w kazdej czesci 
// - ile krawedzi jest przecietych
// - informacje o parametrach algorytmu i czasie wykonania
void print_statistics(const Graph *graph, const Partition_data *partition_data, int parts, float accuracy, int precompute, double execution_time);

#endif
