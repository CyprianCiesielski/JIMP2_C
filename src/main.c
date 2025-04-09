#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "file_reader.h"

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

int main() {
    Graph graph;

    // Wczytaj graf z pliku
    load_graph("data/graf.csrrg", &graph);

    // Wyświetl graf
    printf("Graf w formie list sąsiedztwa:\n");
    print_graph(&graph);

    // Zwolnij pamięć grafu
    for (int i = 0; i < graph.vertices; i++) {
        free(graph.nodes[i].neighbors);
    }
    free(graph.nodes);

    return 0;
}