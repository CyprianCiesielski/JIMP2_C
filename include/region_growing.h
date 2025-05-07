#ifndef REGION_GROWING_H
#define REGION_GROWING_H

#include "graph.h"
#include "partition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

// struktura kolejki do przechodzenia grafu algorytmem BFS
// uzywana przy sprawdzaniu spojnosci partycji
struct Queue
{
    int *items;   // tablica elementow w kolejce
    int front;    // indeks pierwszego elementu
    int rear;     // indeks ostatniego elementu + 1
    int max_size; // maksymalny rozmiar kolejki
};

// sprawdza czy kolejka jest pusta
int is_empty(struct Queue *queue);

// dodaje element na koniec kolejki
void add_to_queue(struct Queue *queue, int item);

// usuwa element z poczatku kolejki
void remove_from_queue(struct Queue *queue);

// glowny algorytm podzialu grafu metoda rozrostu regionow
// zwraca 1 jesli podzial sie powiodl w granicach dokladnosci, 0 w przeciwnym razie
int region_growing(Graph *graph, int parts, Partition_data *partition_data, float accuracy);

// losuje wierzcholki startowe dla kazdej partycji
// zwraca tablice indeksow wierzcholkow startowych
int *generate_seed_points(Graph *graph, int parts);

// sprawdza czy partycje sa spojne
// wypisuje informacje o spojnosci kazdej partycji
void check_partition_connectivity(Graph *graph, int parts);

#endif // REGION_GROWING_H