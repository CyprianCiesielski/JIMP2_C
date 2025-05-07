#include "stats.h"
#include <math.h>

// oblicza odchylenie standardowe
double calculate_std_dev(const int* values, int count, double mean) {
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum += diff * diff;
    }
    return sqrt(sum / count);
}

void print_statistics(const Graph *graph, const Partition_data *partition_data, int parts, float accuracy, int precompute, double execution_time) {
    printf("\n============= Szczegolowa analiza grafu i partycjonowania =============\n");

    // 1. Statystyki grafu
    printf("\n=== Podstawowe statystyki grafu ===\n");
    int total_edges = 0;
    double avg_degree = 0.0;
    int min_degree = graph->vertices;
    int max_degree = 0;
    
    for (int i = 0; i < graph->vertices; i++) {
        int degree = graph->nodes[i].neighbor_count;
        total_edges += degree;
        if (degree < min_degree) min_degree = degree;
        if (degree > max_degree) max_degree = degree;
    }
    total_edges /= 2;
    avg_degree = (double)total_edges * 2 / graph->vertices;

    printf("Wierzcholki i krawedzie:\n");
    printf("- Liczba wierzcholkow: %d\n", graph->vertices);
    printf("- Liczba krawedzi: %d\n", total_edges);
    printf("- Gestosc grafu: %.4f%%\n", 
           (float)(total_edges * 2) / (graph->vertices * (graph->vertices - 1)) * 100);
    
    printf("\nStopnie wierzcholkow:\n");
    printf("- Minimalny stopien: %d\n", min_degree);
    printf("- Maksymalny stopien: %d\n", max_degree);
    printf("- Sredni stopien: %.2f\n", avg_degree);

    // 2. Statystyki partycji
    printf("\n=== Analiza partycji ===\n");
    int *partition_sizes = malloc(parts * sizeof(int));
    double avg_size = (double)graph->vertices / parts;
    int min_size = graph->vertices;
    int max_size = 0;
    
    for (int i = 0; i < parts; i++) {
        int size = partition_data->parts[i].part_vertex_count;
        partition_sizes[i] = size;
        if (size < min_size) min_size = size;
        if (size > max_size) max_size = size;
    }
    
    double size_std_dev = calculate_std_dev(partition_sizes, parts, avg_size);
    
    printf("Rozmiary partycji:\n");
    printf("- Sredni rozmiar: %.2f wierzcholkow\n", avg_size);
    printf("- Najmniejsza partycja: %d wierzcholkow (%.2f%% sredniej)\n", 
           min_size, (min_size/avg_size)*100);
    printf("- Najwieksza partycja: %d wierzcholkow (%.2f%% sredniej)\n", 
           max_size, (max_size/avg_size)*100);
    printf("- Odchylenie standardowe: %.2f wierzcholkow\n", size_std_dev);
    printf("- Wspolczynnik zmiennosci: %.2f%%\n", (size_std_dev/avg_size)*100);

    printf("\nSzczegoly partycji:\n");
    for (int i = 0; i < parts; i++) {
        printf("Partycja %d:\n", i);
        printf("  - Liczba wierzcholkow: %d (%.2f%% grafu)\n", 
               partition_sizes[i], (float)partition_sizes[i]/graph->vertices*100);
    }

    // 3. Analiza przeciec
    printf("\n=== Analiza przeciec ===\n");
    int cut_edges = 0;
    int *partition_cuts = calloc(parts, sizeof(int));
    int max_part_cuts = 0;
    int min_part_cuts = total_edges;
    
    for (int i = 0; i < graph->vertices; i++) {
        int part1 = graph->nodes[i].part_id;
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++) {
            int neighbor = graph->nodes[i].neighbors[j];
            int part2 = graph->nodes[neighbor].part_id;
            if (part1 != part2) {
                cut_edges++;
                partition_cuts[part1]++;
            }
        }
    }
    cut_edges /= 2;

    double avg_part_cuts = (double)cut_edges / parts;
    for (int i = 0; i < parts; i++) {
        partition_cuts[i] /= 2;
        if (partition_cuts[i] > max_part_cuts) max_part_cuts = partition_cuts[i];
        if (partition_cuts[i] < min_part_cuts) min_part_cuts = partition_cuts[i];
    }

    printf("Ogolne statystyki przeciec:\n");
    printf("- Calkowita liczba przecietych krawedzi: %d\n", cut_edges);
    printf("- Procent przecietych krawedzi: %.2f%%\n", (float)cut_edges/total_edges*100);
    printf("- Srednia liczba przeciec na partycje: %.2f\n", avg_part_cuts);
    
    printf("\nPrzeciecia per partycja:\n");
    for (int i = 0; i < parts; i++) {
        printf("Partycja %d:\n", i);
        printf("  - Liczba przecietych krawedzi: %d\n", partition_cuts[i]);
        printf("  - Procent wszystkich przeciec: %.2f%%\n", 
               (float)partition_cuts[i]/cut_edges*100);
    }

    // 4. Wydajnosc
    printf("\n=== Metryki wydajnosci ===\n");
    printf("- Czas wykonania: %.3f sekund\n", execution_time);
    printf("- Sredni czas na wierzcholek: %.6f ms\n", 
           (execution_time * 1000) / graph->vertices);
    printf("- Dokladnosc podzialu: %.2f%%\n", accuracy * 100);
    printf("- Prekomputacja metryk: %s\n", precompute ? "Tak" : "Nie");

    printf("\n================================================================\n");

    // Zwolnij zaalokowana pamiec
    free(partition_sizes);
    free(partition_cuts);
}

void print_precompute_metrics(const Graph *graph, const Partition_data *partition_data, int parts) {
    printf("\n=== Statystyki przed podziałem ===\n");
    
    // Oblicz podstawowe metryki
    int total_edges = 0;
    for (int i = 0; i < graph->vertices; i++) {
        total_edges += graph->nodes[i].neighbor_count;
    }
    total_edges /= 2;
    
    // Statystyki pamieci
    double memory_per_vertex = sizeof(Node);
    double memory_per_edge = sizeof(int) * 2; // każda krawędź jest przechowywana 2 razy
    double memory_per_partition = sizeof(Part); // pamięć na strukturę partycji
    double total_partition_memory = (memory_per_partition * parts + 
                                   sizeof(Partition_data)) / 1024.0; // w KB
    double total_memory = (memory_per_vertex * graph->vertices + 
                         memory_per_edge * total_edges + 
                         total_partition_memory) / 1024.0; // w KB
    
    printf("\nZuzycie pamieci:\n");
    printf("- Calkowita pamiec grafu: %.2f KB\n", total_memory);
    printf("- Pamiec na wierzcholek: %.2f B\n", memory_per_vertex);
    printf("- Pamiec na krawedz: %.2f B\n", memory_per_edge);
    printf("- Pamiec na struktury partycji: %.2f KB\n", total_partition_memory);
    
    // Statystyki struktury grafu
    double avg_degree = (double)total_edges * 2 / graph->vertices;
    printf("\nStruktura grafu:\n");
    printf("- Sredni stopien wierzcholka: %.2f\n", avg_degree);
    printf("- Gestosc grafu: %.4f%%\n", 
           (float)(total_edges * 2) / (graph->vertices * (graph->vertices - 1)) * 100);
    
    // Analiza partycji i alokacji
    int total_partition_vertices = 0;
    int total_partition_capacity = 0;
    for (int i = 0; i < parts; i++) {
        total_partition_vertices += partition_data->parts[i].part_vertex_count;
        total_partition_capacity += partition_data->parts[i].capacity;
    }
    
    printf("\nAnaliza struktur partycji:\n");
    printf("- Liczba czesci: %d\n", parts);
    printf("- Calkowita liczba przypisanych wierzcholkow: %d\n", total_partition_vertices);
    printf("- Sredni rozmiar czesci: %.2f wierzcholkow\n", 
           (double)graph->vertices / parts);
    printf("- Calkowita zarezerwowana pojemnosc: %d\n", total_partition_capacity);
    printf("- Wykorzystanie pamieci partycji: %.2f%%\n", 
           (float)total_partition_vertices / total_partition_capacity * 100);
    
    // Statystyki operacji
    printf("\nOperacje przed podziałem:\n");
    printf("- Inicjalizacja wierzcholkow: %d\n", graph->vertices);
    printf("- Inicjalizacja krawedzi: %d\n", total_edges * 2);
    printf("- Inicjalizacja struktur partycji: %d\n", parts);
    printf("- Calkowita liczba operacji: %d\n", 
           graph->vertices + total_edges * 2 + parts);
           
    printf("\n=== Koniec statystyk przed podziałem ===\n");
}
