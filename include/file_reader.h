#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

// struktura przechowujaca dane wczytane z pliku wejsciowego
// line1 - liczba wierzcholkow (pierwsza linia)
// line2, line2_count - wskazniki do wierszy i ich liczba (druga linia) 
// line3, line3_count - liczby sasiadow i ich ilosc (trzecia linia)
// edges, edge_count - krawedzie i ich liczba (czwarta linia)
// row_pointers, row_count - wskazniki do wierszy i ich liczba (piata linia)
typedef struct
{
    int *line1;
    int *line2;
    int line2_count;
    int *line3;
    int line3_count;
    int *edges;
    int edge_count;
    int *row_pointers;
    int row_count;
} ParsedData;

// dekoduje liczbe zapisana w formacie vbyte (zmienna liczba bajtow)
int decode_vbyte(FILE *file);

// czyta i wyswietla zawartosc pliku binarnego 
void read_binary(const char *filename);

// wczytuje graf z pliku tekstowego do struktury Graph
// korzysta ze struktury ParsedData do przechowania danych posrednich
void load_graph(const char *filename, Graph *graph, ParsedData *data);

// dodaje sasiada do listy sasiadow wierzcholka
// jesli brakuje miejsca to zwieksza bufor
void add_neighbor(Node *node, int neighbor);

#endif