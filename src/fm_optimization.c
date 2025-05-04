#include "fm_optimization.h"
#include "graph.h"
#include "partition.h"

void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    FM_Context *context = initialize_fm_context(graph, partition_data, max_iterations);
    if (!context)
    {
        fprintf(stderr, "Failed to initialize FM context\n");
        return;
    }

    identify_boundary_vertices(context, context->locked);
    context->initial_cut = calculate_initial_cut(context);
    context->current_cut = context->initial_cut;
    context->best_cut = context->initial_cut;

    for (int iter = 0; iter < max_iterations; iter++)
    {
        // Na początku każdej iteracji
        memset(context->locked, 0, context->graph->vertices * sizeof(bool));

        int best_move_vertex = find_best_move(context, context->locked);
        if (best_move_vertex == -1)
        {
            break; // No more valid moves
        }

        apply_move(context, best_move_vertex, context->target_parts[best_move_vertex]);
        update_gains_after_move(context, best_move_vertex);

        if (context->current_cut < context->best_cut)
        {
            context->best_cut = context->current_cut;
            save_best_solution(context);
        }
    }

    print_cut_statistics(context);
    restore_best_solution(context);
    free_fm_context(context);
}

FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    FM_Context *context = malloc(sizeof(FM_Context));
    if (!context)
    {
        fprintf(stderr, "Failed to allocate memory for FM context\n");
        return NULL;
    }

    context->graph = graph;
    context->partition = partition_data;
    context->max_iterations = max_iterations;
    context->iterations = 0;
    context->moves_made = 0;
    context->initial_cut = 0;
    context->current_cut = 0;
    context->best_cut = 0;

    // Allocate memory for locked vertices and gains
    context->locked = malloc(graph->vertices * sizeof(bool));
    if (!context->locked)
    {
        fprintf(stderr, "Failed to allocate memory for locked vertices\n");
        free(context);
        return NULL;
    }
    memset(context->locked, 0, graph->vertices * sizeof(bool));
    context->gains = malloc(graph->vertices * sizeof(int));
    if (!context->gains)
    {
        fprintf(stderr, "Failed to allocate memory for gains\n");
        free(context->locked);
        free(context);
        return NULL;
    }
    context->target_parts = malloc(graph->vertices * sizeof(int));
    if (!context->target_parts)
    {
        fprintf(stderr, "Failed to allocate memory for target parts\n");
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }
    context->part_sizes = malloc(graph->parts * sizeof(int));
    if (!context->part_sizes)
    {
        fprintf(stderr, "Failed to allocate memory for part sizes\n");
        free(context->target_parts);
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }
    for (int i = 0; i < graph->parts; i++)
    {
        context->part_sizes[i] = partition_data->parts[i].part_vertex_count;
    }
    context->best_partition = malloc(graph->vertices * sizeof(int));
    if (!context->best_partition)
    {
        fprintf(stderr, "Failed to allocate memory for best partition\n");
        free(context->part_sizes);
        free(context->target_parts);
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }
    // Initialize gains and target parts
    for (int i = 0; i < context->graph->vertices; i++)
    {
        context->gains[i] = 0;
        context->target_parts[i] = graph->nodes[i].part_id;
    }

    // Po zainicjowaniu tablicy locked
    for (int i = 0; i < context->graph->vertices; i++)
    {
        // Początkowo dla każdego wierzchołka oblicz potencjalny zysk
        for (int p = 0; p < context->graph->parts; p++)
        {
            if (p != context->graph->nodes[i].part_id)
            {
                int gain = calculate_gain(context, i, p);
                if (gain > context->gains[i])
                {
                    context->gains[i] = gain;
                    context->target_parts[i] = p;
                }
            }
        }
    }

    return context;
}

void free_fm_context(FM_Context *context)
{
    if (context)
    {
        free(context->locked);
        free(context->gains);
        free(context->target_parts);
        free(context->part_sizes);
        free(context->best_partition);
        free(context);
    }
}

void identify_boundary_vertices(FM_Context *context, bool *is_boundary)
{
    for (int i = 0; i < context->graph->vertices; i++)
    {
        is_boundary[i] = false;
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                is_boundary[i] = true;
                break;
            }
        }
    }
}

int calculate_initial_cut(FM_Context *context)
{
    int cut_edges = 0;
    for (int i = 0; i < context->graph->vertices; i++)
    {
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }
    return cut_edges / 2; // Each edge is counted twice
}

int calculate_gain(FM_Context *context, int vertex, int target_part)
{
    int current_part = context->graph->nodes[vertex].part_id;
    int gain = 0;

    // Przejrzyj wszystkich sąsiadów wierzchołka
    for (int i = 0; i < context->graph->nodes[vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[vertex].neighbors[i];
        int neighbor_part = context->graph->nodes[neighbor].part_id;

        if (neighbor_part == target_part)
        {
            // Sąsiad jest w docelowej partycji - to zmniejszy liczbę krawędzi przecinających
            gain++;
        }
        else if (neighbor_part == current_part)
        {
            // Sąsiad jest w bieżącej partycji - to zwiększy liczbę krawędzi przecinających
            gain--;
        }
        // Sąsiedzi w innych partycjach nie wpływają na zmianę liczby krawędzi przecinających
    }

    return gain;
}

int is_valid_move(FM_Context *context, int vertex, int target_part)
{
    int current_part = context->graph->nodes[vertex].part_id;

    if (context->locked[vertex])
    {
        return 0; // Vertex is already moved
    }

    if (current_part == target_part)
    {
        return 0; // Vertex is already in the target partition
    }

    // Check balance constraints
    int new_size_source = context->part_sizes[current_part] - 1;
    int new_size_target = context->part_sizes[target_part] + 1;
    int min_size = context->graph->min_count;
    int max_size = context->graph->max_count;

    if (new_size_source < min_size || new_size_target > max_size)
    {
        return 0; // Would violate balance constraints
    }

    return 1;
}

void apply_move(FM_Context *context, int vertex, int target_part)
{
    int current_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // Update partition assignment
    context->graph->nodes[vertex].part_id = target_part;

    // Update partition sizes
    context->part_sizes[current_part]--;
    context->part_sizes[target_part]++;

    // Update cut count and move statistics
    context->current_cut -= gain; // Subtract gain because negative gain means increased cut edges
    context->moves_made++;
    context->locked[vertex] = true; // Lock the vertex after moving
}

void update_gains_after_move(FM_Context *context, int moved_vertex)
{
    for (int i = 0; i < context->graph->nodes[moved_vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[moved_vertex].neighbors[i];
        if (!context->locked[neighbor])
        {
            context->gains[neighbor] = calculate_gain(context, neighbor, context->target_parts[neighbor]);
        }
    }
}

int find_best_move(FM_Context *context, bool *is_boundary)
{
    int best_vertex = -1;
    int best_gain = -1000000; // Use INT_MIN instead of arbitrary large negative number

    for (int i = 0; i < context->graph->vertices; i++)
    {
        if (is_boundary[i] && !context->locked[i])
        {
            // Check possible target partitions (all except the current one)
            for (int p = 0; p < context->graph->parts; p++) // Use parts consistently
            {
                if (p != context->graph->nodes[i].part_id)
                {
                    int gain = calculate_gain(context, i, p);
                    if (gain > best_gain && is_valid_move(context, i, p))
                    {
                        best_gain = gain;
                        best_vertex = i;
                        context->target_parts[i] = p;
                    }
                }
            }
        }
    }

    return best_vertex;
}

void save_best_solution(FM_Context *context)
{
    for (int i = 0; i < context->graph->vertices; i++)
    {
        context->best_partition[i] = context->graph->nodes[i].part_id;
    }
}

void restore_best_solution(FM_Context *context)
{
    // First reset part sizes
    memset(context->part_sizes, 0, context->graph->parts * sizeof(int));

    // Restore partition assignments and update part sizes
    for (int i = 0; i < context->graph->vertices; i++)
    {
        context->graph->nodes[i].part_id = context->best_partition[i];
        context->part_sizes[context->best_partition[i]]++;
    }

    // Reset current cut to best cut
    context->current_cut = context->best_cut;
}

void print_cut_statistics(FM_Context *context)
{
    printf("Initial cut: %d\n", context->initial_cut);
    printf("Current cut: %d\n", context->current_cut);
    printf("Best cut: %d\n", context->best_cut);
    printf("Moves made: %d\n", context->moves_made);
}