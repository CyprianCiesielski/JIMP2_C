#include "graph.h"
#include "partition.h"

int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size) {
    if (!graph || !partition_data || !size) {
        return NULL;
    }

    // Get size of this partition
    *size = partition_data->parts[part_id].part_vertex_count;
    if (*size <= 0) {
        return NULL;
    }

    // Allocate memory for array of neighbor arrays
    int **neighbors = malloc(*size * sizeof(int*));
    if (!neighbors) {
        return NULL;
    }

    // For each vertex in partition
    for (int i = 0; i < *size; i++) {
        int vertex = partition_data->parts[part_id].part_vertexes[i];
        
        // Validate vertex index
        if (vertex < 0 || vertex >= graph->vertices) {
            fprintf(stderr, "Invalid vertex index: %d\n", vertex);
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++) {
                free(neighbors[j]);
            }
            free(neighbors);
            return NULL;
        }

        // Count neighbors in same partition
        int neighbor_count = 0;
        for (int j = 0; j < graph->nodes[vertex].neighbor_count; j++) {
            int neighbor = graph->nodes[vertex].neighbors[j];
            // Check if neighbor is in same partition
            for (int k = 0; k < partition_data->parts[part_id].part_vertex_count; k++) {
                if (partition_data->parts[part_id].part_vertexes[k] == neighbor) {
                    neighbor_count++;
                    break;
                }
            }
        }

        // Allocate array for vertex and its neighbors (+2 for vertex itself and terminator)
        neighbors[i] = malloc((neighbor_count + 2) * sizeof(int));
        if (!neighbors[i]) {
            // Clean up
            for (int j = 0; j < i; j++) {
                free(neighbors[j]);
            }
            free(neighbors);
            return NULL;
        }

        // Store vertex as first element
        neighbors[i][0] = vertex;

        // Store neighbors
        int pos = 1;
        for (int j = 0; j < graph->nodes[vertex].neighbor_count; j++) {
            int neighbor = graph->nodes[vertex].neighbors[j];
            // Check if neighbor is in same partition
            for (int k = 0; k < partition_data->parts[part_id].part_vertex_count; k++) {
                if (partition_data->parts[part_id].part_vertexes[k] == neighbor) {
                    neighbors[i][pos++] = neighbor;
                    break;
                }
            }
        }
        
        // Add terminator
        neighbors[i][pos] = -1;
    }

    return neighbors;
}

void print_part_neighbors(int **neighbors, int size) {
    if (!neighbors || size <= 0) {
        return;
    }

    for (int i = 0; i < size; i++) {
        if (!neighbors[i]) {
            continue;
        }
        printf("Wierzchołek %d: ", neighbors[i][0]);
        int j = 1;
        while (neighbors[i][j] != -1) {
            printf("%d", neighbors[i][j]);
            if (neighbors[i][j + 1] != -1) {
                printf(", ");
            }
            j++;
        }
        printf("\n");
    }
}#include <graph.h>

void inicialize_graph(Graph *graph, int vertices)
{
    graph->vertices = vertices;
    graph->edges = 0;
    graph->parts = 0;
    count_edges(graph);

    graph->nodes = malloc(vertices * sizeof(Node));
    if (graph->nodes == NULL)
    {
        perror("Błąd alokacji pamięci dla węzłów");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < vertices; i++)
    {
        graph->nodes[i].vertex = i;
        graph->nodes[i].neighbors = NULL;
        graph->nodes[i].neighbor_count = 0;
        graph->nodes[i].part_id = -1; // -1 oznacza brak przypisania
        graph->nodes[i].neighbor_capacity = 0;
        graph->nodes[i].gain = NULL;
        graph->nodes[i].bucket_node = NULL;
        graph->nodes[i].bucket_index = -1;
    }
}

void count_edges(Graph *graph)
{
    int edge_count = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        edge_count += graph->nodes[i].neighbor_count;
    }
    graph->edges = edge_count / 2; // Dzielimy przez 2, ponieważ każda krawędź jest liczona dwa razy
}
void assing_parts(Graph *graph, int parts)
{
    graph->parts = parts;
}

void free_graph(Graph *graph)
{
    for (int i = 0; i < graph->vertices; i++)
    {
        free(graph->nodes[i].neighbors);
    }
    free(graph->nodes);
}