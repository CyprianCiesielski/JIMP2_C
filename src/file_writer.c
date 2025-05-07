#include "file_writer.h"
#include <stdint.h> 

int is_in_partition(const Partition_data *partition_data, int part_id, int vertex) {
    for (int i = 0; i < partition_data->parts[part_id].part_vertex_count; i++) {
        if (partition_data->parts[part_id].part_vertexes[i] == vertex) {
            return 1;
        }
    }
    return 0;
}

void get_partition_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int vertex, int *neighbors, int *count) {
    *count = 0;
    
    for (int i = 0; i < graph->nodes[vertex].neighbor_count; i++) {
        int neighbor = graph->nodes[vertex].neighbors[i];
        
        if (is_in_partition(partition_data, part_id, neighbor)) {
            neighbors[*count] = neighbor;
            (*count)++;
        }
    }
}

int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void write_text(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return;
    }

    fprintf(file, "%d\n", *(data->line1));

    for (int i = 0; i < data->line2_count; i++) {
        fprintf(file, "%d", data->line2[i]);
        if (i < data->line2_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    for (int i = 0; i < data->line3_count; i++) {
        fprintf(file, "%d", data->line3[i]);
        if (i < data->line3_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    int *sizes = malloc(parts * sizeof(int));
    int ***all_part_neighbors = malloc(parts * sizeof(int**));
    
    if (!sizes || !all_part_neighbors) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (int part = 0; part < parts; part++) {
        all_part_neighbors[part] = get_part_neighbors(graph, partition_data, part, &sizes[part]);
        if (!all_part_neighbors[part]) {
            fprintf(stderr, "Failed to get neighbors for part %d\n", part);
            goto cleanup;
        }
    }

    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            fprintf(file, "%d", all_part_neighbors[part][i][0]);
            
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                if (j == 1) {
                    fprintf(file, ";");
                } else {
                    fprintf(file, ",");
                }
                fprintf(file, "%d", all_part_neighbors[part][i][j]);
                j++;
            }

            if (!(part == parts - 1 && i == sizes[part] - 1)) {
                fprintf(file, ";");
            }
        }
    }
    fprintf(file, "\n");

    fprintf(file, "0");
    int last_pos = 0;

    for (int i = 0; i < sizes[0]; i++) {
        int neighbor_count = 0;
        int j = 1;
        while (all_part_neighbors[0][i][j] != -1) {
            neighbor_count++;
            j++;
        }
        last_pos += (neighbor_count + 1);
        fprintf(file, ";%d", last_pos);
    }
    fprintf(file, "\n");

    for (int part = 1; part < parts; part++) {
        fprintf(file, "%d", last_pos);
        int pos = last_pos;

        for (int i = 0; i < sizes[part]; i++) {
            int neighbor_count = 0;
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                neighbor_count++;
                j++;
            }
            pos += (neighbor_count + 1);
            fprintf(file, ";%d", pos);
        }
        last_pos = pos;
        fprintf(file, "\n");
    }

cleanup:
    if (all_part_neighbors) {
        for (int part = 0; part < parts; part++) {
            if (all_part_neighbors[part]) {
                for (int i = 0; i < sizes[part]; i++) {
                    free(all_part_neighbors[part][i]);
                }
                free(all_part_neighbors[part]);
            }
        }
        free(all_part_neighbors);
    }
    free(sizes);
    fclose(file);
}

// Funkcja pomocnicza do kodowania liczby w formacie vByte
void encode_vbyte(FILE *file, int value) {
    while (value >= 128) {
        uint8_t byte = (value & 0x7F) | 0x80; // 7 bitów wartości + MSB = 1
        fwrite(&byte, sizeof(uint8_t), 1, file);
        value >>= 7; // Przesunięcie o 7 bitów
    }
    uint8_t byte = value & 0x7F; // Ostatni bajt z MSB = 0
    fwrite(&byte, sizeof(uint8_t), 1, file);
}

void write_binary(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return;
    }

    const uint64_t separator = 0xDEADBEEFCAFEBABE;

    encode_vbyte(file, *(data->line1));
    fwrite(&separator, sizeof(uint64_t), 1, file);

    for (int i = 0; i < data->line2_count; i++) {
        encode_vbyte(file, data->line2[i]);
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    for (int i = 0; i < data->line3_count; i++) {
        encode_vbyte(file, data->line3[i]);
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    int *sizes = malloc(parts * sizeof(int));
    int ***all_part_neighbors = malloc(parts * sizeof(int**));
    
    if (!sizes || !all_part_neighbors) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return;
    }

    for (int part = 0; part < parts; part++) {
        all_part_neighbors[part] = get_part_neighbors(graph, partition_data, part, &sizes[part]);
        if (!all_part_neighbors[part]) {
            fprintf(stderr, "Failed to get neighbors for part %d\n", part);
            goto cleanup;
        }
    }

    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            encode_vbyte(file, all_part_neighbors[part][i][0]);
            
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                encode_vbyte(file, all_part_neighbors[part][i][j]);
                j++;
            }
        }
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    encode_vbyte(file, 0);
    int last_pos = 0;

    for (int i = 0; i < sizes[0]; i++) {
        int neighbor_count = 0;
        int j = 1;
        while (all_part_neighbors[0][i][j] != -1) {
            neighbor_count++;
            j++;
        }
        last_pos += (neighbor_count + 1);
        encode_vbyte(file, last_pos);
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    for (int part = 1; part < parts; part++) {
        encode_vbyte(file, last_pos);
        int pos = last_pos;

        for (int i = 0; i < sizes[part]; i++) {
            int neighbor_count = 0;
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                neighbor_count++;
                j++;
            }
            pos += (neighbor_count + 1);
            encode_vbyte(file, pos);
        }
        last_pos = pos;
        if (part < parts - 1) {
            fwrite(&separator, sizeof(uint64_t), 1, file);
        }
    }

cleanup:
    // Free allocated memory
    if (all_part_neighbors) {
        for (int part = 0; part < parts; part++) {
            if (all_part_neighbors[part]) {
                for (int i = 0; i < sizes[part]; i++) {
                    free(all_part_neighbors[part][i]);
                }
                free(all_part_neighbors[part]);
            }
        }
        free(all_part_neighbors);
    }
    free(sizes);
    fclose(file);
}