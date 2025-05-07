#include "stats.h"

// wyswietla statystyki dotyczace grafu, podzialu i parametrow algorytmu
void print_statistics(const Graph *graph, const Partition_data *partition_data, int parts, float accuracy, int precompute, double execution_time)
{
    printf("\n=== Statystyki programu ===\n");

    // podstawowe info o grafie
    printf("\nStatystyki grafu:\n"); 
    printf("- Liczba wierzcholkow: %d\n", graph->vertices);

    // policz wszystkie krawedzie
    int total_edges = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        total_edges += graph->nodes[i].neighbor_count;
    }
    printf("- Liczba krawedzi: %d\n", total_edges / 2);

    // info o podziale grafu
    printf("\nStatystyki podzialu:\n");
    printf("- Liczba czesci: %d\n", parts);
    printf("- Dokladnosc podzialu: %.2f%%\n", accuracy * 100);

    // wielkosc kazdej czesci
    printf("\nRozmiary czesci:\n");
    for (int i = 0; i < parts; i++)
    {
        printf("- Czesc %d: %d wierzcholkow\n", i, partition_data->parts[i].part_vertex_count);
    }

    // policz ile krawedzi jest przecietych
    int cut_edges = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        int part1 = graph->nodes[i].part_id;
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = graph->nodes[i].neighbors[j];
            int part2 = graph->nodes[neighbor].part_id;
            if (part1 != part2)
            {
                cut_edges++;
            }
        }
    }
    cut_edges /= 2;

    // statystyki dotyczace przecietych krawedzi
    printf("\nStatystyki przeciec:\n");
    printf("- Przeciete krawedzie: %d\n", cut_edges);
    printf("- Procent przecietych krawedzi: %.2f%%\n", (float)cut_edges / (total_edges / 2) * 100);

    printf("- Obliczanie metryk: %s\n", precompute ? "Tak" : "Nie");

    // czas wykonania
    printf("\nWydajnosc:\n");
    printf("- Czas wykonania: %.3f sekund\n", execution_time);
}
