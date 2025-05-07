#include "partition.h"
#include "graph.h"
#include <string.h>

static int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

static int is_in_partition(const Partition_data *partition_data, int part_id, int vertex) {
    if (!partition_data || part_id < 0 || part_id >= partition_data->parts_count) {
        return 0;
    }
    
    for (int i = 0; i < partition_data->parts[part_id].part_vertex_count; i++) {
        if (partition_data->parts[part_id].part_vertexes[i] == vertex) {
            return 1;
        }
    }
    return 0;
}

int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size) {
    if (!graph || !partition_data || !size || part_id < 0) {
        return NULL;
    }

    *size = partition_data->parts[part_id].part_vertex_count;
    if (*size <= 0) {
        return NULL;
    }

    int *vertices = malloc(*size * sizeof(int));
    if (!vertices) {
        return NULL;
    }

    memcpy(vertices, partition_data->parts[part_id].part_vertexes, *size * sizeof(int));
    qsort(vertices, *size, sizeof(int), compare_ints);

    int **neighbors = malloc(*size * sizeof(int*));
    if (!neighbors) {
        free(vertices);
        return NULL;
    }

    for (int i = 0; i < *size; i++) {
        int vertex = vertices[i];
        int max_neighbors = graph->nodes[vertex].neighbor_count;
        
        int *temp_neighbors = malloc(max_neighbors * sizeof(int));
        int neighbor_count = 0;

        if (!temp_neighbors) {
            for (int j = 0; j < i; j++) {
                free(neighbors[j]);
            }
            free(neighbors);
            free(vertices);
            return NULL;
        }

        for (int j = 0; j < max_neighbors; j++) {
            int neighbor = graph->nodes[vertex].neighbors[j];
            if (is_in_partition(partition_data, part_id, neighbor)) {
                temp_neighbors[neighbor_count++] = neighbor;
            }
        }

        if (neighbor_count > 0) {
            qsort(temp_neighbors, neighbor_count, sizeof(int), compare_ints);
        }

        neighbors[i] = malloc((neighbor_count + 2) * sizeof(int));
        if (!neighbors[i]) {
            for (int j = 0; j < i; j++) {
                free(neighbors[j]);
            }
            free(neighbors);
            free(vertices);
            free(temp_neighbors);
            return NULL;
        }

        neighbors[i][0] = vertex;
        
        for (int j = 0; j < neighbor_count; j++) {
            neighbors[i][j + 1] = temp_neighbors[j];
        }
        
        neighbors[i][neighbor_count + 1] = -1;

        free(temp_neighbors);
    }

    free(vertices);
    return neighbors;
}

void initialize_partition_data(Partition_data *partition_data, int parts)
{
    if (!partition_data)
    {
        fprintf(stderr, "Null partition_data pointer\n");
        return;
    }

    partition_data->parts_count = parts;
    partition_data->parts = malloc(parts * sizeof(Part));
    if (!partition_data->parts)
    {
        perror("Memory allocation error for parts");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < parts; i++)
    {
        partition_data->parts[i].part_id = i;
        partition_data->parts[i].part_vertexes = NULL;
        partition_data->parts[i].part_vertex_count = 0;
        partition_data->parts[i].capacity = 0;
    }
}

void free_partition_data(Partition_data *partition_data, int parts) {
    if (!partition_data) {
        return;
    }

    if (partition_data->parts) {
        for (int i = 0; i < parts; i++) {
            free(partition_data->parts[i].part_vertexes);
        }
        free(partition_data->parts);
    }
}

void print_partition_data(const Partition_data *partition_data)
{
    if (!partition_data)
        return;

    printf("Number of parts: %d\n", partition_data->parts_count);

    for (int i = 0; i < partition_data->parts_count; i++)
    {
        printf("Part %d: ", i);
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++)
        {
            printf("%d ", partition_data->parts[i].part_vertexes[j]);
        }
        printf("\n");
    }

    printf("\nVertex count in each part:\n");
    for (int i = 0; i < partition_data->parts_count; i++)
    {
        printf("Part %d: %d\n", i, partition_data->parts[i].part_vertex_count);
    }
}

void add_partition_data(Partition_data *partition_data, int part_id, int vertex)
{
    if (!partition_data || part_id < 0 || part_id >= partition_data->parts_count)
    {
        fprintf(stderr, "Invalid partition data or part_id\n");
        return;
    }

    if (partition_data->parts[part_id].part_vertexes == NULL)
    {
        partition_data->parts[part_id].capacity = 128;
        partition_data->parts[part_id].part_vertexes = malloc(partition_data->parts[part_id].capacity * sizeof(int));
        if (!partition_data->parts[part_id].part_vertexes)
        {
            perror("Memory allocation error for part vertices");
            exit(EXIT_FAILURE);
        }
        partition_data->parts[part_id].part_vertex_count = 0;
    }
    else if (partition_data->parts[part_id].part_vertex_count >= partition_data->parts[part_id].capacity)
    {
        partition_data->parts[part_id].capacity *= 2;
        int *new_vertexes = realloc(partition_data->parts[part_id].part_vertexes,
                                    partition_data->parts[part_id].capacity * sizeof(int));
        if (!new_vertexes)
        {
            perror("Memory reallocation error for part vertices");
            exit(EXIT_FAILURE);
        }
        partition_data->parts[part_id].part_vertexes = new_vertexes;
    }

    partition_data->parts[part_id].part_vertexes[partition_data->parts[part_id].part_vertex_count++] = vertex;
}

