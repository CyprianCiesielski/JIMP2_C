#include "partition.h"
#include "graph.h"

int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size)
{
    if (!graph || !partition_data || !size)
    {
        return NULL;
    }

    // Get size of this partition
    *size = partition_data->parts[part_id].part_vertex_count;
    if (*size <= 0)
    {
        return NULL;
    }

    // Allocate memory for array of neighbor arrays
    int **neighbors = malloc(*size * sizeof(int *));
    if (!neighbors)
    {
        return NULL;
    }

    // For each vertex in partition
    for (int i = 0; i < *size; i++)
    {
        int vertex = partition_data->parts[part_id].part_vertexes[i];

        // Validate vertex index
        if (vertex < 0 || vertex >= graph->vertices)
        {
            fprintf(stderr, "Invalid vertex index: %d\n", vertex);
            // Clean up previously allocated memory
            for (int j = 0; j < i; j++)
            {
                free(neighbors[j]);
            }
            free(neighbors);
            return NULL;
        }

        // Count neighbors in same partition
        int neighbor_count = 0;
        for (int j = 0; j < graph->nodes[vertex].neighbor_count; j++)
        {
            int neighbor = graph->nodes[vertex].neighbors[j];
            // Check if neighbor is in same partition
            for (int k = 0; k < partition_data->parts[part_id].part_vertex_count; k++)
            {
                if (partition_data->parts[part_id].part_vertexes[k] == neighbor)
                {
                    neighbor_count++;
                    break;
                }
            }
        }

        // Allocate array for vertex and its neighbors (+2 for vertex itself and terminator)
        neighbors[i] = malloc((neighbor_count + 2) * sizeof(int));
        if (!neighbors[i])
        {
            // Clean up
            for (int j = 0; j < i; j++)
            {
                free(neighbors[j]);
            }
            free(neighbors);
            return NULL;
        }

        // Store vertex as first element
        neighbors[i][0] = vertex;

        // Store neighbors
        int pos = 1;
        for (int j = 0; j < graph->nodes[vertex].neighbor_count; j++)
        {
            int neighbor = graph->nodes[vertex].neighbors[j];
            // Check if neighbor is in same partition
            for (int k = 0; k < partition_data->parts[part_id].part_vertex_count; k++)
            {
                if (partition_data->parts[part_id].part_vertexes[k] == neighbor)
                {
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

void free_partition_data(Partition_data *partition_data)
{
    if (!partition_data)
        return;

    for (int i = 0; i < partition_data->parts_count; i++)
    {
        free(partition_data->parts[i].part_vertexes);
    }
    free(partition_data->parts);
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

    // Check if we need to initialize the vertex array
    if (partition_data->parts[part_id].part_vertexes == NULL)
    {
        // Initial allocation with capacity for 128 elements
        partition_data->parts[part_id].capacity = 128;
        partition_data->parts[part_id].part_vertexes = malloc(partition_data->parts[part_id].capacity * sizeof(int));
        if (!partition_data->parts[part_id].part_vertexes)
        {
            perror("Memory allocation error for part vertices");
            exit(EXIT_FAILURE);
        }
        partition_data->parts[part_id].part_vertex_count = 0;
    }
    // Check if we need to expand the array
    else if (partition_data->parts[part_id].part_vertex_count >= partition_data->parts[part_id].capacity)
    {
        // Double the capacity
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

    // Add the new vertex
    partition_data->parts[part_id].part_vertexes[partition_data->parts[part_id].part_vertex_count++] = vertex;
}
