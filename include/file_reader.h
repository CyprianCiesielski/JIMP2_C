#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *line1;          // Pierwsza linia (jedna liczba)
    int *line2;          // Druga linia (tablica liczb)
    int line2_count;     // Liczba elementów w line2
    int *line3;          // Trzecia linia (tablica liczb)
    int line3_count;     // Liczba elementów w line3
    int *edges;          // Czwarta linia (tablica krawędzi)
    int edge_count;      // Liczba elementów w edges
    int *row_pointers;   // Piąta linia (tablica wskaźników wierszy)
    int row_count;       // Liczba elementów w row_pointers
} ParsedData;

int decode_vbyte(FILE *file);
void read_binary(const char *filename);
void load_graph(const char *filename, Graph *graph, ParsedData *data);
void add_neighbor(Node *node, int neighbor);

#endif