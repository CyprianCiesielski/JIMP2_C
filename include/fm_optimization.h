#ifndef FM_OPTIMIZATION_H
#define FM_OPTIMIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "graph.h"
#include "partition.h"
#include "region_growing.h"

// deklaracje zapowiadajace, zeby uniknac cyklicznych zaleznosci
struct Graph;
typedef struct Graph Graph;
struct Partition_data;
typedef struct Partition_data Partition_data;

// struktura opisujaca potencjalny ruch wierzcholka miedzy partycjami
typedef struct
{
    int vertex;      // indeks wierzcholka
    int gain;        // zysk z przeniesienia (zmniejszenie liczby krawedzi przecinajacych)
    int target_part; // docelowa partycja
} Move;

// struktura do przechowywania potencjalnych ruchow pogrupowanych wedlug zysku
typedef struct
{
    Move **moves; // tablica ruchow dla kazdego mozliwego zysku
    int max_gain; // maksymalny mozliwy zysk
    int min_gain; // minimalny mozliwy zysk
} Bucket;

// struktura przechowujaca dane kontekstowe dla algorytmu FM
typedef struct
{
    Graph *graph;              // wskaznik na graf
    Partition_data *partition; // wskaznik na dane partycji
    bool *locked;              // tablica wierzcholkow zablokowanych (juz przesunietych)
    int *gains;                // zyski dla kazdego wierzcholka
    int *target_parts;         // docelowe partycje dla kazdego wierzcholka
    int max_iterations;        // maksymalna liczba iteracji
    int *part_sizes;           // biezace rozmiary partycji
    int iterations;            // liczba wykonanych iteracji
    int moves_made;            // liczba wykonanych przesuniec
    int initial_cut;           // poczatkowa liczba krawedzi przekrojowych
    int current_cut;           // biezaca liczba krawedzi przekrojowych
    int best_cut;              // najlepsza znaleziona liczba krawedzi przecinajacych
    int *best_partition;       // najlepszy znaleziony podzial
} FM_Context;

// glowna funkcja optymalizacji algorytmem Fiduccia-Mattheysa
void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations);

// tworzy i inicjalizuje strukture kontekstowa dla algorytmu FM
FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations);

// zwalnia pamiec zaalokowana dla kontekstu FM
void free_fm_context(FM_Context *context);

// znajduje wierzcholki graniczne (majace sasiadow w innych partycjach)
void identify_boundary_vertices(FM_Context *context, bool *is_boundary);

// oblicza poczatkowa liczbe krawedzi przecinajacych partycje
int calculate_initial_cut(FM_Context *context);

// oblicza zysk z przeniesienia wierzcholka do innej partycji
int calculate_gain(FM_Context *context, int vertex, int target_part);

// sprawdza czy mozna przeniesc wierzcholek do docelowej partycji
int is_valid_move(FM_Context *context, int vertex, int target_part);

// wykonuje ruch wierzcholka do innej partycji
void apply_move(FM_Context *context, int vertex, int target_part);

// aktualizuje zyski po wykonaniu ruchu
void update_gains_after_move(FM_Context *context, int moved_vertex);

// znajduje najlepszy mozliwy ruch w obecnym stanie
int find_best_move(FM_Context *context, bool *is_boundary);

// wyswietla statystyki optymalizacji
void print_cut_statistics(FM_Context *context);

// zapisuje najlepsze znalezione rozwiazanie
void save_best_solution(FM_Context *context);

// przywraca najlepsze znalezione rozwiazanie
void restore_best_solution(FM_Context *context);

// sprawdza czy usuniecie wierzcholka z partycji nie naruszy jej spojnosci
int will_remain_connected_if_removed(Graph *graph, int vertex);

// sprawdza czy ruch jest dozwolony z zachowaniem spojnosci
int is_move_valid_with_integrity(FM_Context *context, int vertex, int target_part);

// wykonuje ruch z zachowaniem spojnosci partycji
int apply_move_safely(FM_Context *context, int vertex, int target_part);

//

#endif // FM_OPTIMIZATION_H
