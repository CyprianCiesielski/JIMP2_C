#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "file_reader.h"
#include "partition.h"
#include "graph.h"
#include <stdint.h> 

// maksymalna liczba sasiadow jaka moze miec wierzcholek
#define MAX_NEIGHBORS 10000000

// zapisuje podzielony graf do pliku tekstowego w formacie csrrg
// filename - nazwa pliku do zapisu
// data - dane wejsciowe z pliku
// partition_data - informacje o podziale grafu
// graph - graf do zapisania
// parts - liczba czesci na ktore podzielono graf
void write_text(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts);

// zapisuje podzielony graf do pliku binarnego
// podobne parametry co write_text ale zapisuje w formacie binarnym z vbyte
void write_binary(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts);

// koduje liczbe w formacie vbyte (zmienna liczba bajtow)
// im mniejsza liczba tym mniej bajtow potrzeba
void encode_vbyte(FILE *file, int value);

// znajduje wszystkich sasiadow wierzcholka ktorzy sa w tej samej czesci grafu
// zwraca przez parametry neighbors i count
void get_partition_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int vertex, int *neighbors, int *count);

// sprawdza czy dany wierzcholek nalezy do danej czesci grafu
// zwraca 1 jesli tak, 0 jesli nie
int is_in_partition(const Partition_data *partition_data, int part_id, int vertex);

#endif