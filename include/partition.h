#ifndef PARTITION_H
#define PARTITION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

// deklaracje zapowiadajace, zeby uniknac cyklicznych zaleznosci
struct Graph;
typedef struct Graph Graph;

// struktura opisujaca pojedyncza czesc grafu
typedef struct Part
{
    int part_id;           // identyfikator czesci
    int part_vertex_count; // liczba wierzcholkow w czesci
    int *part_vertexes;    // tablica indeksow wierzcholkow w tej czesci
    int capacity;          // aktualna pojemnosc tablicy (do dynamicznego powiekszania)
} Part;

// struktura przechowujaca dane o calym podziale grafu na czesci
typedef struct Partition_data
{
    int parts_count; // liczba czesci w podziale
    Part *parts;     // tablica wszystkich czesci
} Partition_data;

// inicjalizuje strukture danych partycji z okreslona liczba czesci
void initialize_partition_data(Partition_data *partition_data, int parts);

// zwalnia pamiec zaalokowana dla struktury partycji
void free_partition_data(Partition_data *partition_data, int parts);

// dodaje wierzcholek o podanym indeksie do wskazanej czesci
void add_partition_data(Partition_data *partition_data, int part_id, int vertex);

// znajduje wszystkich sasiadow wierzcholkow w danej czesci grafu
// zwraca tablice tablic sasiadow dla kazdego wierzcholka
int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size);

// wypisuje informacje o podziale grafu na czesci
void print_partition_data(const Partition_data *partition_data);

#endif