#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_graph(const char *filename, Graph *graph);
void add_neighbor(Node *node, int neighbor);

#endif