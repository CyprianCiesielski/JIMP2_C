#ifndef FM_OPTIMIZATION_H
#define FM_OPTIMIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Forward declarations - zastępują #include "graph.h" i #include "partition.h"
struct Graph;
typedef struct Graph Graph;
struct Partition_data;
typedef struct Partition_data Partition_data;

// Struktura opisująca zysk z przeniesienia wierzchołka
typedef struct
{
    int vertex;      // Indeks wierzchołka
    int gain;        // Zysk z przeniesienia (zmniejszenie liczby krawędzi przekrojowych)
    int target_part; // Docelowa partycja
} Move;

// Struktura typu "bucket" do przechowywania potencjalnych ruchów
typedef struct
{
    Move **moves; // Tablica ruchów dla każdego możliwego zysku
    int max_gain; // Maksymalny możliwy zysk
    int min_gain; // Minimalny możliwy zysk
} Bucket;

// Struktura kontekstu optymalizacji
typedef struct
{
    Graph *graph;              // Wskaźnik na graf
    Partition_data *partition; // Wskaźnik na dane partycji
    bool *locked;              // Tablica wierzchołków zablokowanych (już przesuniętych)
    int *gains;                // Zyski dla każdego wierzchołka
    int *target_parts;         // Docelowe partycje dla każdego wierzchołka
    int max_iterations;        // Maksymalna liczba iteracji
    int *part_sizes;           // Bieżące rozmiary partycji
    int iterations;            // Liczba wykonanych iteracji
    int moves_made;            // Liczba wykonanych przesunięć
    int initial_cut;           // Początkowa liczba krawędzi przekrojowych
    int current_cut;           // Bieżąca liczba krawędzi przekrojowych
    int best_cut;              // Najlepsza znaleziona liczba krawędzi przekrojowych
    int *best_partition;       // Najlepszy znaleziony podział
} FM_Context;

// Główna funkcja optymalizacji algorytmem Fiducia-Mattheysa
void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations);

// Funkcje pomocnicze
FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations);
void free_fm_context(FM_Context *context);
void identify_boundary_vertices(FM_Context *context, bool *is_boundary);
int calculate_initial_cut(FM_Context *context);
int calculate_gain(FM_Context *context, int vertex, int target_part);
int is_valid_move(FM_Context *context, int vertex, int target_part);
void apply_move(FM_Context *context, int vertex, int target_part);
void update_gains_after_move(FM_Context *context, int moved_vertex);
int find_best_move(FM_Context *context, bool *is_boundary);
void print_cut_statistics(FM_Context *context);
void save_best_solution(FM_Context *context);
void restore_best_solution(FM_Context *context);

#endif // FM_OPTIMIZATION_H
