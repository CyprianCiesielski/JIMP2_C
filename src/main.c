#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "graph.h"
#include "file_reader.h"
#include "file_writer.h"
#include "region_growing.h"
#include "partition.h"
#include <time.h>
#include "stats.h"

// Funkcja do wyświetlania grafu
void print_graph(const Graph *graph)
{
    for (int i = 0; i < graph->vertices; i++)
    {
        printf("Wierzchołek %d: ", graph->nodes[i].vertex);
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            printf("%d, ", graph->nodes[i].neighbors[j]);
        }
        printf("\n");
    }
}

void print_data(const Graph *graph, const Partition_data *partition_data, int parts) {
    printf("\nPodział grafu na %d części:\n", parts);
    for (int i = 0; i < parts; i++) {
        printf("\nCzęść %d:\n", i);
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++) {
            int current_vertex = partition_data->parts[i].part_vertexes[j];
            printf("Wierzchołek %d: ", current_vertex);
            
            // Znajdź sąsiadów w tej samej części
            int neighbor_count = 0;
            for (int k = 0; k < graph->nodes[current_vertex].neighbor_count; k++) {
                int neighbor = graph->nodes[current_vertex].neighbors[k];
                
                // Sprawdź czy sąsiad jest w tej samej części
                for (int l = 0; l < partition_data->parts[i].part_vertex_count; l++) {
                    if (partition_data->parts[i].part_vertexes[l] == neighbor) {
                        if (neighbor_count > 0) {
                            printf(", ");
                        }
                        printf("%d", neighbor);
                        neighbor_count++;
                        break;
                    }
                }
            }
            printf("\n");
        }
    }
}

void print_usage(char *program_name) {
    printf("Usage: %s [parts] [accuracy] [input_file] [options]\n", program_name);
    printf("\nPositional arguments:\n");
    printf("  parts              Number of parts (default: 2)\n");
    printf("  accuracy          Accuracy percentage with %% sign (default: 10%%)\n");
    printf("  input_file        Input file name (default: graf.csrrg)\n");
    printf("\nOptions:\n");
    printf("  --max-edges-cut N  Maximum number of edges that can be cut\n");
    printf("  --precompute-metrics  Precompute metrics before partitioning\n");
    printf("  --output FILE      Output file name (default: anwser.csrrg)\n");
    printf("  -h, --help         Show this help message\n");
}

int main(int argc, char *argv[]) {
    // Default values
    char path[256];
    char *percent;  // Moved declaration outside switch
    int parts = 2;
    float accuracy = 0.1;
    char *input_file = "graf.csrrg";
    char *output_file = "anwser";
    int max_edges_cut = -1;
    int precompute = 0;

    // Handle help flag first if present
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Handle positional arguments first
    int pos_args = 0;
    int i = 1;
    while (i < argc && argv[i][0] != '-') {
        switch(pos_args) {
            case 0:
                parts = atoi(argv[i]);
                if (parts <= 0) {
                    fprintf(stderr, "Number of parts must be positive\n");
                    return 1;
                }
                break;
            case 1:
                percent = strchr(argv[i], '%');  // Removed declaration
                if (percent) *percent = '\0';
                accuracy = atof(argv[i]) / 100.0;
                if (accuracy <= 0 || accuracy > 1) {
                    fprintf(stderr, "Accuracy must be between 1 and 100\n");
                    return 1;
                }
                break;
            case 2:
                input_file = argv[i];
                break;
        }
        pos_args++;
        i++;
    }

    // Handle optional flags
    while (i < argc) {
        if (strcmp(argv[i], "--precompute-metrics") == 0 || strcmp(argv[i], "-p") == 0) {
            precompute = 1;
            i++;
        }
        else if ((strcmp(argv[i], "--output") == 0 && i + 1 < argc) || (strcmp(argv[i], "-o") == 0 && i + 1 < argc)) {
            output_file = argv[i + 1];
            i += 2;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }

    // Create input path
    snprintf(path, sizeof(path), "data/%s", input_file);

    // Rest of your code...
    clock_t start = clock();
    Graph graph;
    ParsedData data = {0};
    Partition_data partition_data;
    initialize_partition_data(&partition_data, parts);

    load_graph(path, &graph, &data);
    region_growing(&graph, parts, &partition_data, accuracy);
    check_partition_connectivity(&graph, parts);

    // Use output_file for writing text and binary formats
    char output_path[256];
    char binary_path[256];

    // Create paths for text and binary outputs separately
    snprintf(output_path, sizeof(output_path), "data/%s.csrrg", output_file);
    snprintf(binary_path, sizeof(binary_path), "data/%s.bin", output_file);

    // Write both formats
    write_text(output_path, &data, &partition_data, &graph, parts);
    write_binary(binary_path, &data, &partition_data, &graph, parts);

    // Use max_edges_cut and precompute to avoid unused warnings
    if (max_edges_cut >= 0) {
        printf("Maximum edges to cut: %d\n", max_edges_cut);
    }
    if (precompute) {
        printf("Precomputing metrics enabled\n");
    }

    // ... rest of cleanup code ...
    clock_t end = clock();
    double execution_time = (double)(end - start) / CLOCKS_PER_SEC;

    if (precompute) {
        print_statistics(&graph, &partition_data, parts, accuracy, max_edges_cut, precompute, execution_time);
    }
    // Free allocated memory
    for (int i = 0; i < graph.vertices; i++) {
        free(graph.nodes[i].neighbors);
    }
    free(graph.nodes);
    free(data.line1);
    free(data.line2);
    free(data.line3);
    free(data.row_pointers);
    free(data.edges);
    for (int i = 0; i < parts; i++) {
        free(partition_data.parts[i].part_vertexes);
    }
    
    return 0;
}