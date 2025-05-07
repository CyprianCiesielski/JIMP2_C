#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "partition.h"

// struktura reprezentujaca wierzcholek grafu
// zawiera informacje o sasiadach i przydzielonej partycji
typedef struct Node
{
    int vertex;            // indeks wierzcholka
    int *neighbors;        // tablica sasiadow (indeksow innych wierzcholkow)
    int neighbor_count;    // liczba sasiadow
    int part_id;           // numer przypisanej partycji (-1 jesli brak)
    int neighbor_capacity; // pojemnosc tablicy sasiadow (do dynamicznej alokacji)
} Node;

// struktura reprezentujaca caly graf
// przechowuje tablice wierzcholkow i ich polaczenia
typedef struct Graph
{
    int vertices;  // liczba wierzcholkow
    int edges;     // liczba krawedzi
    int parts;     // liczba czesci podzialu grafu
    int min_count; // minimalna liczba wierzcholkow w czesci
    int max_count; // maksymalna liczba wierzcholkow w czesci
    Node *nodes;   // tablica wszystkich wierzcholkow
} Graph;

// wypisuje sasiadow dla wierzcholkow partycji
void print_part_neighbors(int **neighbors, int size);

// tworzy nowy graf o podanej liczbie wierzcholkow
void inicialize_graph(Graph *graph, int vertices);

// oblicza liczbe krawedzi w grafie
void count_edges(Graph *graph);

// ustala minimalna i maksymalna liczbe wierzcholkow na czesc
void assign_min_max_count(Graph *graph, int parts, float accuracy);

// ustawia liczbe czesci grafu
void assing_parts(Graph *graph, int parts);

// zwalnia pamiec zajmowana przez graf
void free_graph(Graph *graph);

#endif