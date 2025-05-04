#include "graph.h"
#include "partition.h"


void print_part_neighbors(int **neighbors, int size)
{
    if (!neighbors || size <= 0)
    {
        return;
    }

    for (int i = 0; i < size; i++)
    {
        if (!neighbors[i])
        {
            continue;
        }
        printf("Wierzchołek %d: ", neighbors[i][0]);
        int j = 1;
        while (neighbors[i][j] != -1)
        {
            printf("%d", neighbors[i][j]);
            if (neighbors[i][j + 1] != -1)
            {
                printf(", ");
            }
            j++;
        }
        printf("\n");
    }
}

void inicialize_graph(Graph *graph, int vertices)
{
    graph->vertices = vertices;
    graph->edges = 0;
    graph->parts = 0;
    graph->min_count = 0;
    graph->max_count = 0;
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
void assign_min_max_count(Graph *graph, int parts, float accuracy)
{
    if (parts <= 0 || accuracy < 0.0 || accuracy > 1.0)
    {
        fprintf(stderr, "Invalid parts or accuracy value\n");
        return;
    }

    graph->parts = parts;

    // Obliczanie średniej liczby wierzchołków na czę

    float avg_vertices_per_part = (float)graph->vertices / parts;

    // Obliczanie min i max liczby wierzchołków na podstawie accuracy
    int min_vertices_per_part = (int)((avg_vertices_per_part * (1.0 - accuracy)));
    if ((float)min_vertices_per_part < (avg_vertices_per_part * (1.0 - accuracy)))
    {
        min_vertices_per_part += 1;
    }
    int max_vertices_per_part = (int)(avg_vertices_per_part * (1.0 + accuracy));
    graph->min_count = min_vertices_per_part;
    graph->max_count = max_vertices_per_part;
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