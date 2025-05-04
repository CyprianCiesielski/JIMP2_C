#include <graph.h>

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