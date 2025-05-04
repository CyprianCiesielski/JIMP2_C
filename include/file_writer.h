#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "file_reader.h"
#include "partition.h"
#include "graph.h"

#define MAX_NEIGHBORS 1000

void write_text(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts);
void write_binary(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts);
void encode_vbyte(FILE *file, int value);
void get_partition_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int vertex, int *neighbors, int *count);
int is_in_partition(const Partition_data *partition_data, int part_id, int vertex);

#endif // FILE_WRITER_H