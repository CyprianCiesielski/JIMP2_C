#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include <stdio.h>
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

#endif // FILE_READER_H

#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "file_reader.h"

void write_text(const char *filename, const ParsedData *data);
void write_binary(const char *filename, const ParsedData *data);
void encode_vbyte(FILE *file, int value);

#endif // FILE_WRITER_H