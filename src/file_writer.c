#include "file_writer.h"
#include <stdint.h> 
// jak zapisywac grafy???

// Helper function to check if vertex is in given partition
int is_in_partition(const Partition_data *partition_data, int part_id, int vertex) {
    for (int i = 0; i < partition_data->parts[part_id].part_vertex_count; i++) {
        if (partition_data->parts[part_id].part_vertexes[i] == vertex) {
            return 1;
        }
    }
    return 0;
}

// Helper function to get neighbors from the same partition
void get_partition_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int vertex, int *neighbors, int *count) {
    *count = 0;
    
    // For each neighbor of the vertex
    for (int i = 0; i < graph->nodes[vertex].neighbor_count; i++) {
        int neighbor = graph->nodes[vertex].neighbors[i];
        
        // Check if this neighbor is in the same partition
        if (is_in_partition(partition_data, part_id, neighbor)) {
            neighbors[*count] = neighbor;
            (*count)++;
        }
    }
}

// Funkcja do zapisu danych do pliku tekstowego
void write_text(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return;
    }

    // Zapis pierwszej linii
    fprintf(file, "%d\n", *(data->line1));

    // Zapis drugiej linii
    for (int i = 0; i < data->line2_count; i++) {
        fprintf(file, "%d", data->line2[i]);
        if (i < data->line2_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    // Zapis trzeciej linii
    for (int i = 0; i < data->line3_count; i++) {
        fprintf(file, "%d", data->line3[i]);
        if (i < data->line3_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    // Store all partition neighbors before writing
    int *sizes = malloc(parts * sizeof(int));
    int ***all_part_neighbors = malloc(parts * sizeof(int**));
    if (!sizes || !all_part_neighbors) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    // Get neighbors for all parts
    for (int part = 0; part < parts; part++) {
        all_part_neighbors[part] = get_part_neighbors(graph, partition_data, part, &sizes[part]);
        if (!all_part_neighbors[part]) {
            fprintf(stderr, "Failed to get neighbors for part %d\n", part);
            // Clean up previously allocated memory
            for (int j = 0; j < part; j++) {
                for (int i = 0; i < sizes[j]; i++) {
                    free(all_part_neighbors[j][i]);
                }
                free(all_part_neighbors[j]);
            }
            free(all_part_neighbors);
            free(sizes);
            return;
        }
    }

    // Write fourth line using stored data
    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            fprintf(file, "%d", all_part_neighbors[part][i][0]);
            
            if (all_part_neighbors[part][i][1] != -1) {
                fprintf(file, ";");
                int j = 1;
                while (all_part_neighbors[part][i][j] != -1) {
                    fprintf(file, "%d", all_part_neighbors[part][i][j]);
                    if (all_part_neighbors[part][i][j + 1] != -1) {
                        fprintf(file, ",");
                    }
                    j++;
                }
            }
            
            if (!(part == parts - 1 && i == sizes[part] - 1)) {
                fprintf(file, ";");
            }
        }
    }
    fprintf(file, "\n");

    // Write indices for all parts
    int last_sum = 0; // Keep track of last sum from previous line

    // For each part
    for (int part = 0; part < parts; part++) {
        // Write starting position (last sum from previous part)
        fprintf(file, "%d", last_sum);
        
        int current_sum = last_sum; // Start counting from last position
        
        // For each vertex in current part
        for (int i = 0; i < sizes[part]; i++) {
            // Count neighbors for current vertex
            int neighbor_count = 0;
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                neighbor_count++;
                j++;
            }
            
            // Add to cumulative sum and write
            current_sum += (neighbor_count + 1);
            fprintf(file, ";%d", current_sum);
        }
        
        fprintf(file, "\n");
        last_sum = current_sum; // Save last sum for next part
    }

    // Free all allocated memory
    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            free(all_part_neighbors[part][i]);
        }
        free(all_part_neighbors[part]);
    }
    free(all_part_neighbors);
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

// Zapisywanie do pliku binarnego
void write_binary(const char *filename, const ParsedData *data, const Partition_data *partition_data, const Graph *graph, int parts) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Nie można otworzyć pliku binarnego do zapisu");
        exit(EXIT_FAILURE);
    }

    const uint64_t separator = 0xDEADBEEFCAFEBABE;

    // Zapis pierwszej linii
    encode_vbyte(file, *(data->line1));
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Zapis drugiej linii
    for (int i = 0; i < data->line2_count; i++) {
        encode_vbyte(file, data->line2[i]);
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Zapis trzeciej linii
    for (int i = 0; i < data->line3_count; i++) {
        encode_vbyte(file, data->line3[i]);
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Store all partition neighbors before writing
    int *sizes = malloc(parts * sizeof(int));
    int ***all_part_neighbors = malloc(parts * sizeof(int**));
    if (!sizes || !all_part_neighbors) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    // Get neighbors for all parts
    for (int part = 0; part < parts; part++) {
        all_part_neighbors[part] = get_part_neighbors(graph, partition_data, part, &sizes[part]);
        if (!all_part_neighbors[part]) {
            fprintf(stderr, "Failed to get neighbors for part %d\n", part);
            // Clean up previously allocated memory
            for (int j = 0; j < part; j++) {
                for (int i = 0; i < sizes[j]; i++) {
                    free(all_part_neighbors[j][i]);
                }
                free(all_part_neighbors[j]);
            }
            free(all_part_neighbors);
            free(sizes);
            return;
        }
    }

    // Write fourth line
    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            // Write vertex
            encode_vbyte(file, all_part_neighbors[part][i][0]);
            
            // Write neighbors if they exist
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                encode_vbyte(file, all_part_neighbors[part][i][j]);
                j++;
            }
        }
    }
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Write indices for all parts
    int last_sum = 0;
    for (int part = 0; part < parts; part++) {
        // Write starting position
        encode_vbyte(file, last_sum);
        
        int current_sum = last_sum;
        
        // Calculate and write cumulative sums for this part
        for (int i = 0; i < sizes[part]; i++) {
            int neighbor_count = 0;
            int j = 1;
            while (all_part_neighbors[part][i][j] != -1) {
                neighbor_count++;
                j++;
            }
            
            current_sum += (neighbor_count + 1);
            encode_vbyte(file, current_sum);
        }
        
        if (part < parts - 1) {
            fwrite(&separator, sizeof(uint64_t), 1, file);
        }
        last_sum = current_sum;
    }

    // Free allocated memory
    for (int part = 0; part < parts; part++) {
        for (int i = 0; i < sizes[part]; i++) {
            free(all_part_neighbors[part][i]);
        }
        free(all_part_neighbors[part]);
    }
    free(all_part_neighbors);
    free(sizes);

    fclose(file);
}