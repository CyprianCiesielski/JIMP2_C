#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "file_reader.h"
#include "file_writer.h"
#warning "file_writer.h included successfully"

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
    ParsedData data = {0}; // Inicjalizacja struktury ParsedData

    // Wczytaj graf z pliku i sparsuj dane
    load_graph("data/graf.csrrg", &graph, &data);

    // Zapisz dane do plików
    write_text("data/answer.csrrg", &data);
    write_binary("data/answer.bin", &data);

    // Odczytaj i wyświetl dane z pliku binarnego
    printf("\nOdczyt danych z pliku binarnego:\n");
    read_binary("data/answer.bin");

    // Zwolnij pamięć
    for (int i = 0; i < graph.vertices; i++) {
        free(graph.nodes[i].neighbors);
    }
    free(graph.nodes);
    free(data.line1);
    free(data.line2);
    free(data.line3);
    free(data.edges);
    free(data.row_pointers);

    return 0;
}