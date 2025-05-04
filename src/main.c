#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "file_reader.h"
#include "file_writer.h"
#include "region_growing.h"
#include "partition.h"

// Funkcja do wyświetlania grafu
void print_graph(const Graph *graph)
{
    for (int i = 0; i < graph->vertices; i++)
    {
        printf("Wierzchołek %d: ", graph->nodes[i].vertex);
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            printf("%d, ", graph->nodes[i].neighbors[j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    char path[256];
    int parts = argc > 1 ? atoi(argv[1]) : 2;
    float accuracy = argc > 2 ? atof(argv[2]) / 100.0 : 0.1;
    char *file = argc > 3 ? argv[3] : "graf.csrrg";
    snprintf(path, sizeof(path), "data/%s", file);

    Graph graph;
    ParsedData data = {0};                             // Inicjalizacja struktury ParsedData
    Partition_data partition_data;                     // Inicjalizacja struktury Partition_data
    initialize_partition_data(&partition_data, parts); // Inicjalizacja danych o częściach

    // Wczytaj graf z pliku i sparsuj dane
    load_graph(path, &graph, &data);

    // Wypisz dane o grafie
    printf("Wczytany graf:\n");
    // print_graph(&graph);

    // 1. podzial grafu na czesci
    region_growing(&graph, parts, &partition_data, accuracy);

    check_partition_connectivity(&graph, parts); // Sprawdzenie spójności partycji
    // print_partition_data(&partition_data, parts); // Wypisz dane o częściach

    // 2. optymalizacja krawędzi cięcia
    optimize_cut_edges_FM(&graph, &partition_data, parts, 0.1);
    count_cut_edges(&graph, &partition_data, parts); // Zliczanie krawędzi cięcia

    
    printf("ilość wierzchołkow: %d", graph.vertices);
    free(graph.nodes);
    free(data.line1);
    free(data.line2);
    free(data.line3);
    free(data.edges);
    free(data.row_pointers);

    return 0;
}