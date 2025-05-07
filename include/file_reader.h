#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

int decode_vbyte(FILE *file);
void read_binary(const char *filename);
void load_graph(const char *filename, Graph *graph, ParsedData *data);
void add_neighbor(Node *node, int neighbor);

#endif