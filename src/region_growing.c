#include "region_growing.h"

int is_empty(struct Queue *queue)
{
    return queue->front == queue->rear;
}

void add_to_queue(struct Queue *queue, int item)
{
    if (queue->rear < queue->max_size)
    {
        queue->items[queue->rear++] = item;
    }
    else
    {
        fprintf(stderr, "Kolejka jest pełna\n");
    }
}

void remove_from_queue(struct Queue *queue)
{
    if (!is_empty(queue))
    {
        queue->front++;
    }
    else
    {
        fprintf(stderr, "Kolejka jest pusta\n");
    }
}

int *generate_seed_points(Graph *graph, int parts)
{
    srand(time(NULL));
    int *seed_points = malloc(parts * sizeof(int));
    if (seed_points == NULL)
    {
        perror("Błąd alokacji pamięci dla punktów startowych");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < parts; i++)
    {
        seed_points[i] = rand() % graph->vertices;
    }

    for (int i = 0; i < parts; i++)
    {
        graph->nodes[seed_points[i]].part_id = i;
    }

    return seed_points;
}

int region_growing(Graph *graph, int parts, Partition_data *partition_data, float accuracy)
{
    if (parts > graph->vertices)
    {
        fprintf(stderr, "Liczba części nie może być większa od liczby wierzchołków\n");
        exit(EXIT_FAILURE);
    }

    int *seed_points = generate_seed_points(graph, parts);
    int *visited = malloc(graph->vertices * sizeof(int));

    if (visited == NULL)
    {
        perror("Błąd alokacji pamięci dla odwiedzonych węzłów");
        free(seed_points);
        exit(EXIT_FAILURE);
    }

    memset(visited, 0, graph->vertices * sizeof(int));

    int **frontier = malloc(parts * sizeof(int *));
    int *frontier_size = malloc(parts * sizeof(int));
    int *frontier_capacity = malloc(parts * sizeof(int));

    if (!frontier || !frontier_size || !frontier_capacity)
    {
        perror("Błąd alokacji pamięci dla frontów partycji");
        free(seed_points);
        free(visited);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < parts; i++)
    {
        frontier_size[i] = 0;
        frontier_capacity[i] = 10;
        frontier[i] = malloc(frontier_capacity[i] * sizeof(int));
        if (!frontier[i])
        {
            perror("Błąd alokacji pamięci dla frontu partycji");
            for (int j = 0; j < i; j++)
            {
                free(frontier[j]);
            }
            free(frontier);
            free(frontier_size);
            free(frontier_capacity);
            free(seed_points);
            free(visited);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < graph->vertices; i++)
    {
        graph->nodes[i].part_id = -1;
    }

    int *part_counts = calloc(parts, sizeof(int));
    if (!part_counts)
    {
        perror("Błąd alokacji pamięci dla liczników partycji");
        for (int i = 0; i < parts; i++)
        {
            free(frontier[i]);
        }
        free(frontier);
        free(frontier_size);
        free(frontier_capacity);
        free(seed_points);
        free(visited);
        exit(EXIT_FAILURE);
    }

    printf("Seed points: ");
    for (int i = 0; i < parts; i++)
    {
        printf("%d ", seed_points[i]);
        visited[seed_points[i]] = 1;
        graph->nodes[seed_points[i]].part_id = i;
        add_partition_data(partition_data, i, seed_points[i]);
        part_counts[i]++;

        Node *node = &graph->nodes[seed_points[i]];
        for (int j = 0; j < node->neighbor_count; j++)
        {
            int neighbor = node->neighbors[j];
            if (!visited[neighbor])
            {
                if (frontier_size[i] >= frontier_capacity[i])
                {
                    frontier_capacity[i] *= 2;
                    frontier[i] = realloc(frontier[i], frontier_capacity[i] * sizeof(int));
                    if (!frontier[i])
                    {
                        perror("Błąd realokacji frontu partycji");
                        exit(EXIT_FAILURE);
                    }
                }
                frontier[i][frontier_size[i]++] = neighbor;
            }
        }
    }
    printf("\n");

    printf("Neighbor counts: ");
    for (int i = 0; i < parts; i++)
    {
        printf("seed %d: %d neighbors, ", seed_points[i], graph->nodes[seed_points[i]].neighbor_count);
    }
    printf("\n");

    float avg_vertices_per_part = (float)graph->vertices / parts;

    int min_vertices_per_part = (int)((avg_vertices_per_part * (1.0 - accuracy)));
    if ((float)min_vertices_per_part < (avg_vertices_per_part * (1.0 - accuracy)))
    {
        min_vertices_per_part += 1;
    }
    int max_vertices_per_part = (int)(avg_vertices_per_part * (1.0 + accuracy));

    printf("Average vertices per part: %.2f (min: %d, max: %d)\n",
           avg_vertices_per_part, min_vertices_per_part, max_vertices_per_part);

    int iterations = 0;
    int unassigned = graph->vertices - parts;

    while (unassigned > 0 && iterations < graph->vertices * 2)
    {
        int min_part = -1;
        for (int i = 0; i < parts; i++)
        {
            if (frontier_size[i] > 0 && part_counts[i] < max_vertices_per_part)
            {
                if (min_part == -1 || part_counts[i] < part_counts[min_part])
                {
                    min_part = i;
                }
            }
        }

        if (min_part == -1)
        {
            int any_frontier = 0;
            for (int i = 0; i < parts; i++)
            {
                if (frontier_size[i] > 0)
                {
                    any_frontier = 1;
                    min_part = i;
                    break;
                }
            }

            if (!any_frontier)
            {
                printf("No available frontiers, terminating\n");
                break;
            }

            if (part_counts[min_part] >= max_vertices_per_part)
            {
                max_vertices_per_part++;
                printf("Increasing max_vertices_per_part to %d\n", max_vertices_per_part);
            }
        }

        int current = frontier[min_part][--frontier_size[min_part]];

        if (!visited[current])
        {
            visited[current] = 1;
            graph->nodes[current].part_id = min_part;
            add_partition_data(partition_data, min_part, current);
            part_counts[min_part]++;
            unassigned--;

            Node *node = &graph->nodes[current];
            for (int i = 0; i < node->neighbor_count; i++)
            {
                int neighbor = node->neighbors[i];

                if (neighbor < 0 || neighbor >= graph->vertices)
                {
                    printf("ERROR: Invalid neighbor index %d for node %d\n", neighbor, current);
                    continue;
                }

                if (!visited[neighbor])
                {
                    if (frontier_size[min_part] >= frontier_capacity[min_part])
                    {
                        frontier_capacity[min_part] *= 2;
                        frontier[min_part] = realloc(frontier[min_part],
                                                     frontier_capacity[min_part] * sizeof(int));
                        if (!frontier[min_part])
                        {
                            perror("Błąd realokacji frontu partycji");
                            exit(EXIT_FAILURE);
                        }
                    }
                    frontier[min_part][frontier_size[min_part]++] = neighbor;
                }
            }
        }

        iterations++;

        if (iterations % 100 == 0)
        {
            int min_count = graph->vertices;
            int max_count = 0;

            for (int i = 0; i < parts; i++)
            {
                if (part_counts[i] < min_count)
                    min_count = part_counts[i];
                if (part_counts[i] > max_count)
                    max_count = part_counts[i];
            }

            if (min_count >= min_vertices_per_part && unassigned == 0)
            {
                printf("Good balance achieved, terminating\n");
                break;
            }
        }
    }

    int unassigned_count = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == -1)
        {
            unassigned_count++;
        }
    }

    if (unassigned_count > 0)
    {
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == -1)
            {
                Node *node = &graph->nodes[i];
                int has_neighbor_in_partition = 0;
                int neighbor_part = -1;
                int smallest_neighbor_part = -1;
                int smallest_neighbor_count = graph->vertices;

                for (int j = 0; j < node->neighbor_count; j++)
                {
                    int neighbor = node->neighbors[j];
                    if (neighbor >= 0 && neighbor < graph->vertices && graph->nodes[neighbor].part_id != -1)
                    {
                        has_neighbor_in_partition = 1;
                        neighbor_part = graph->nodes[neighbor].part_id;

                        if (part_counts[neighbor_part] < smallest_neighbor_count)
                        {
                            smallest_neighbor_count = part_counts[neighbor_part];
                            smallest_neighbor_part = neighbor_part;
                        }
                    }
                }

                if (has_neighbor_in_partition && part_counts[smallest_neighbor_part] < max_vertices_per_part)
                {
                    graph->nodes[i].part_id = smallest_neighbor_part;
                    add_partition_data(partition_data, smallest_neighbor_part, i);
                    part_counts[smallest_neighbor_part]++;
                }
                else
                {
                    int min_part = 0;
                    for (int j = 1; j < parts; j++)
                    {
                        if (part_counts[j] < part_counts[min_part])
                        {
                            min_part = j;
                        }
                    }

                    graph->nodes[i].part_id = min_part;
                    add_partition_data(partition_data, min_part, i);
                    part_counts[min_part]++;
                }
            }
        }
    }

    if (iterations >= graph->vertices * 3)
    {
        printf("WARNING: Loop terminated due to iteration limit\n");
    }

    printf("Final partition sizes: ");
    for (int i = 0; i < parts; i++)
    {
        printf("part %d: %d nodes (%.2f%% of average), ",
               i, part_counts[i], (part_counts[i] * 100.0) / avg_vertices_per_part);
    }
    printf("\n");

    int success = 1;
    float min_ratio = graph->vertices;
    float max_ratio = 0;

    for (int i = 0; i < parts; i++)
    {
        float ratio = (float)part_counts[i] / avg_vertices_per_part;
        if (ratio < min_ratio)
            min_ratio = ratio;
        if (ratio > max_ratio)
            max_ratio = ratio;
    }

    if (min_ratio < (1.0 - accuracy) || max_ratio > (1.0 + accuracy))
    {
        success = 0;
        printf("Warning: Final partition ratios (min: %.2f, max: %.2f) outside accuracy range (%.2f - %.2f)\n",
               min_ratio, max_ratio, 1.0 - accuracy, 1.0 + accuracy);
    }

    free(visited);
    free(seed_points);
    free(part_counts);
    for (int i = 0; i < parts; i++)
    {
        free(frontier[i]);
    }
    free(frontier);
    free(frontier_size);
    free(frontier_capacity);

    return success;
}

void check_partition_connectivity(Graph *graph, int parts)
{
    printf("\n--- Partition Connectivity Check ---\n");
    int flag = 1;
    for (int part = 0; part < parts; part++)
    {
        printf("Checking partition %d:\n", part);

        int *part_vertices = malloc(graph->vertices * sizeof(int));
        int part_size = 0;

        if (!part_vertices)
        {
            perror("Failed to allocate memory for partition vertices");
            return;
        }

        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == part)
            {
                part_vertices[part_size++] = i;
            }
        }

        if (part_size == 0)
        {
            printf("  Partition %d is empty!\n", part);
            free(part_vertices);
            continue;
        }

        int *visited = calloc(graph->vertices, sizeof(int));
        if (!visited)
        {
            perror("Failed to allocate memory for visited array");
            free(part_vertices);
            return;
        }

        int start_vertex = part_vertices[0];
        struct Queue queue = {.front = 0, .rear = 0, .max_size = graph->vertices};
        queue.items = malloc(graph->vertices * sizeof(int));

        if (!queue.items)
        {
            perror("Failed to allocate memory for queue");
            free(part_vertices);
            free(visited);
            return;
        }

        add_to_queue(&queue, start_vertex);
        visited[start_vertex] = 1;
        int connected_vertices = 1;

        while (!is_empty(&queue))
        {
            int current = queue.items[queue.front];
            remove_from_queue(&queue);

            Node *node = &graph->nodes[current];

            for (int i = 0; i < node->neighbor_count; i++)
            {
                int neighbor = node->neighbors[i];

                if (neighbor >= 0 && neighbor < graph->vertices &&
                    graph->nodes[neighbor].part_id == part &&
                    !visited[neighbor])
                {
                    visited[neighbor] = 1;
                    add_to_queue(&queue, neighbor);
                    connected_vertices++;
                }
            }
        }

        if (connected_vertices == part_size)
        {
            printf("  Partition %d is connected. All %d vertices are reachable.\n", part, part_size);
        }
        else
        {
            flag = 0;
            printf("  WARNING: Partition %d is NOT connected! Only %d out of %d vertices are reachable.\n",
                   part, connected_vertices, part_size);

            printf("  Disconnected vertices: ");
            for (int i = 0; i < part_size; i++)
            {
                int vertex = part_vertices[i];
                if (!visited[vertex])
                {
                    printf("%d ", vertex);
                }
            }
            printf("\n");
        }

        printf("  Checking for isolated vertices in partition %d...\n", part);
        int isolated_count = 0;

        for (int i = 0; i < part_size; i++)
        {
            int vertex = part_vertices[i];
            Node *node = &graph->nodes[vertex];
            int has_part_neighbor = 0;

            for (int j = 0; j < node->neighbor_count; j++)
            {
                int neighbor = node->neighbors[j];
                if (neighbor >= 0 && neighbor < graph->vertices &&
                    graph->nodes[neighbor].part_id == part)
                {
                    has_part_neighbor = 1;
                    break;
                }
            }

            if (!has_part_neighbor && node->neighbor_count > 0)
            {
                if (isolated_count == 0)
                {
                    printf("  Isolated vertices: ");
                }
                printf("%d ", vertex);
                isolated_count++;
            }
        }

        if (isolated_count == 0)
        {
            printf("  No isolated vertices found in partition %d.\n", part);
        }
        else
        {
            printf("\n  Found %d isolated vertices in partition %d.\n", isolated_count, part);
        }

        free(queue.items);
        free(visited);
        free(part_vertices);
    }

    printf("--- End of Connectivity Check --- %d\n\n", flag);
}