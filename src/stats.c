#include "stats.h"

void print_statistics(const Graph *graph, const Partition_data *partition_data, int parts, float accuracy, int max_edges_cut, int precompute, double execution_time) {
    printf("\n=== Program Statistics ===\n");
    
    // Graph statistics
    printf("\nGraph Statistics:\n");
    printf("- Total vertices: %d\n", graph->vertices);
    
    int total_edges = 0;
    for (int i = 0; i < graph->vertices; i++) {
        total_edges += graph->nodes[i].neighbor_count;
    }
    printf("- Total edges: %d\n", total_edges / 2);  // Dzielimy przez 2, bo każda krawędź jest liczona dwukrotnie
    
    // Partition statistics
    printf("\nPartition Statistics:\n");
    printf("- Number of parts: %d\n", parts);
    printf("- Target accuracy: %.2f%%\n", accuracy * 100);
    
    // Size of each partition
    printf("\nPartition Sizes:\n");
    for (int i = 0; i < parts; i++) {
        printf("- Part %d: %d vertices\n", i, partition_data->parts[i].part_vertex_count);
    }
    
    // Count edges between partitions
    int cut_edges = 0;
    for (int i = 0; i < graph->vertices; i++) {
        int part1 = graph->nodes[i].part_id;
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++) {
            int neighbor = graph->nodes[i].neighbors[j];
            int part2 = graph->nodes[neighbor].part_id;
            if (part1 != part2) {
                cut_edges++;
            }
        }
    }
    cut_edges /= 2;  // Każda krawędź była liczona dwukrotnie
    
    printf("\nCut Statistics:\n");
    printf("- Cut edges: %d\n", cut_edges);
    printf("- Percentage of cut edges: %.2f%%\n", (float)cut_edges / (total_edges / 2) * 100);
    
    // Algorithm parameters
    printf("\nAlgorithm Parameters:\n");
    if (max_edges_cut >= 0) {
        printf("- Maximum edges that can be cut: %d\n", max_edges_cut);
    }
    printf("- Precompute metrics: %s\n", precompute ? "Yes" : "No");
    
    // Performance statistics
    printf("\nPerformance:\n");
    printf("- Execution time: %.3f seconds\n", execution_time);
}