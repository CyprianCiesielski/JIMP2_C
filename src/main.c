#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "file_reader.h"
#include "file_writer.h"

// Funkcja do wyświetlania grafu
void print_graph(const Graph *graph) {
    for (int i = 0; i < graph->vertices; i++) {
        printf("Wierzchołek %d: ", graph->nodes[i].vertex);
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++) {
            printf("%d, ", graph->nodes[i].neighbors[j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    char path[256];
    int parts = argc > 1 ? atoi(argv[1]) : 2;
    double accuracy = argc > 2 ? atof(argv[2]) / 100.0 : 0.1;
    char *file = argc > 3 ? argv[3] : "graf.cssrg";
    snprintf(path, sizeof(path), "data/%s", file);

    Graph graph;
    ParsedData data = {0}; // Inicjalizacja struktury ParsedData

    // Wczytaj graf z pliku i sparsuj dane
    load_graph(path, &graph, &data);
    
    free(graph.nodes);
    free(data.line1);
    free(data.line2);
    free(data.line3);
    free(data.edges);
    free(data.row_pointers);

    return 0;
}