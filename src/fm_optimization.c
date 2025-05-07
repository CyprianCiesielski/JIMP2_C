#include "fm_optimization.h"
#include "graph.h"
#include "partition.h"
#include <pthread.h>
#include <unistd.h> // Dodano dla sysconf

// Struktura danych dla wątku szukającego najlepszego ruchu
typedef struct
{
    FM_Context *context;
    bool *is_boundary;
    int start_vertex;
    int end_vertex;
    int best_vertex;
    int best_gain;
    int best_target_part;
} ThreadFindMoveData;

void *thread_find_best_move(void *arg)
{
    ThreadFindMoveData *data = (ThreadFindMoveData *)arg;
    FM_Context *context = data->context;
    bool *is_boundary = data->is_boundary;

    data->best_vertex = -1;
    data->best_gain = 0;

    // Sprawdź wszystkie wierzchołki w przydzielonym zakresie
    for (int i = data->start_vertex; i < data->end_vertex; i++)
    {
        // Rozważ tylko wierzchołki niezablokowane i graniczne
        if (is_boundary[i] && !context->locked[i])
        {
            // Sprawdź możliwe docelowe partycje (wszystkie oprócz bieżącej)
            for (int p = 0; p < context->graph->parts; p++)
            {
                if (p != context->graph->nodes[i].part_id)
                {
                    int gain = calculate_gain(context, i, p);
                    // Sprawdź czy ruch ma dodatni zysk
                    if (gain > 0 && gain > data->best_gain && is_valid_move(context, i, p))
                    {
                        data->best_gain = gain;
                        data->best_vertex = i;
                        data->best_target_part = p;
                    }
                }
            }
        }
    }

    return NULL;
}

// Zmodyfikowana funkcja find_best_move używająca wątków
int find_best_move(FM_Context *context, bool *is_boundary)
{
    // Użyj optymalnej liczby wątków dla procesora
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[num_threads];
    ThreadFindMoveData thread_data[num_threads];

    int vertices_per_thread = context->graph->vertices / num_threads;
    int remaining = context->graph->vertices % num_threads;

    // Utwórz wątki, każdy pracujący na swojej części wierzchołków
    for (int t = 0; t < num_threads; t++)
    {
        thread_data[t].context = context;
        thread_data[t].is_boundary = is_boundary;
        thread_data[t].start_vertex = t * vertices_per_thread;
        thread_data[t].end_vertex = (t + 1) * vertices_per_thread;

        // Ostatni wątek dostaje pozostałe wierzchołki
        if (t == num_threads - 1)
            thread_data[t].end_vertex += remaining;

        if (pthread_create(&threads[t], NULL, thread_find_best_move, &thread_data[t]) != 0)
        {
            fprintf(stderr, "Error creating thread %d\n", t);
            // Fallback - wykonaj sekwencyjnie
            for (int i = 0; i < t; i++)
            {
                pthread_cancel(threads[i]);
            }

            // Wykonaj starą wersję find_best_move
            int best_vertex = -1;
            int best_gain = 0;

            for (int i = 0; i < context->graph->vertices; i++)
            {
                if (is_boundary[i] && !context->locked[i])
                {
                    for (int p = 0; p < context->graph->parts; p++)
                    {
                        if (p != context->graph->nodes[i].part_id)
                        {
                            int gain = calculate_gain(context, i, p);
                            if (gain > 0 && gain > best_gain && is_move_valid_with_integrity(context, i, p))
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
    }

    // Czekaj na zakończenie wszystkich wątków
    for (int t = 0; t < num_threads; t++)
    {
        pthread_join(threads[t], NULL);
    }

    // Znajdź najlepszy wynik spośród wszystkich wątków
    int best_vertex = -1;
    int best_gain = 0;
    int best_target_part = -1;

    for (int t = 0; t < num_threads; t++)
    {
        if (thread_data[t].best_vertex != -1 && thread_data[t].best_gain > best_gain)
        {
            best_vertex = thread_data[t].best_vertex;
            best_gain = thread_data[t].best_gain;
            best_target_part = thread_data[t].best_target_part;
        }
    }

    // Ustaw docelową partycję dla wybranego wierzchołka
    if (best_vertex != -1)
    {
        context->target_parts[best_vertex] = best_target_part;
    }

    return best_vertex;
}

int is_partition_connected(Graph *graph, int part_id)
{
    if (!graph || part_id < 0 || part_id >= graph->parts)
        return 0;

    // Liczba wierzchołków w danej partycji
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
        if (graph->nodes[i].part_id == part_id)
            vertices_in_part++;

    if (vertices_in_part == 0)
        return 1; // Pusta partycja jest spójna według definicji

    // Tablica odwiedzonych wierzchołków
    bool *visited = calloc(graph->vertices, sizeof(bool));
    if (!visited)
        return 0;

    // BFS do sprawdzenia spójności
    int *queue = malloc(graph->vertices * sizeof(int));
    if (!queue)
    {
        free(visited);
        return 0;
    }

    // Znajdź pierwszy wierzchołek w partycji
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id)
        {
            start_vertex = i;
            break;
        }
    }

    int front = 0, rear = 0;
    queue[rear++] = start_vertex;
    visited[start_vertex] = true;
    int nodes_visited = 1;

    while (front < rear)
    {
        int current = queue[front++];

        for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
        {
            int neighbor = graph->nodes[current].neighbors[i];

            // Rozważ tylko sąsiadów z tej samej partycji
            if (graph->nodes[neighbor].part_id == part_id && !visited[neighbor])
            {
                visited[neighbor] = true;
                queue[rear++] = neighbor;
                nodes_visited++;
            }
        }
    }

    free(queue);
    free(visited);

    return (nodes_visited == vertices_in_part);
}

int will_remain_connected_if_removed(Graph *graph, int vertex)
{
    // Dodaj więcej informacji diagnostycznych
    printf("Checking if removing vertex %d will preserve connectivity...\n", vertex);

    int current_part = graph->nodes[vertex].part_id;

    // Policz wierzchołki w partycji (bez przenoszonego wierzchołka)
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (i != vertex && graph->nodes[i].part_id == current_part)
            vertices_in_part++;
    }

    printf("Part %d has %d vertices (excluding vertex %d)\n",
           current_part, vertices_in_part, vertex);

    // Jeśli nie ma innych wierzchołków lub tylko jeden, to partycja pozostanie spójna
    if (vertices_in_part <= 1)
    {
        printf("Part has <= 1 vertex, so it remains connected\n");
        return 1;
    }

    // Pozostała część funkcji...
    int original_part = graph->nodes[vertex].part_id;
    graph->nodes[vertex].part_id = -1;

    // Tablice dla BFS
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *queue = malloc(graph->vertices * sizeof(int));

    if (!visited || !queue)
    {
        if (visited)
            free(visited);
        if (queue)
            free(queue);
        graph->nodes[vertex].part_id = original_part; // Przywróć oryginalną partycję
        return 0;
    }

    // Znajdź pierwszy wierzchołek w partycji (inny niż usuwany)
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == original_part)
        {
            start_vertex = i;
            break;
        }
    }

    // BFS
    int front = 0, rear = 0;
    queue[rear++] = start_vertex;
    visited[start_vertex] = true;
    int nodes_visited = 1;

    while (front < rear)
    {
        int current = queue[front++];

        // Sprawdź sąsiadów
        for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
        {
            int neighbor = graph->nodes[current].neighbors[i];

            if (graph->nodes[neighbor].part_id == original_part && !visited[neighbor])
            {
                visited[neighbor] = true;
                queue[rear++] = neighbor;
                nodes_visited++;
            }
        }
    }

    // Przywróć oryginalną partycję wierzchołka
    graph->nodes[vertex].part_id = original_part;

    // Sprawdź czy wszystkie wierzchołki w partycji zostały odwiedzone
    int result = (nodes_visited == vertices_in_part);

    free(visited);
    free(queue);

    return result;
}

int verify_partition_integrity(Graph *graph)
{
    if (!graph)
        return 0;

    int all_connected = 1;

    printf("\n--- Partition Integrity Verification ---\n");
    for (int i = 0; i < graph->parts; i++)
    {
        int is_connected = is_partition_connected(graph, i);
        printf("Partition %d is %s\n", i, is_connected ? "connected" : "DISCONNECTED");

        if (!is_connected)
            all_connected = 0;
    }

    printf("Overall partition integrity: %s\n", all_connected ? "VALID" : "INVALID");
    printf("--- End of Verification ---\n");

    return all_connected;
}

int count_cut_edges(Graph *graph)
{
    if (!graph)
        return 0;

    int cut_edges = 0;

    for (int i = 0; i < graph->vertices; i++)
    {
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = graph->nodes[i].neighbors[j];

            // Liczymy tylko krawędzie w jedną stronę (i < neighbor)
            // aby nie liczyć dwukrotnie tych samych krawędzi
            if (i < neighbor && graph->nodes[i].part_id != graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }

    return cut_edges;
}

void print_final_statistics(Graph *graph)
{
    if (!graph)
        return;

    int actual_cut_edges = count_cut_edges(graph);
    int partition_integrity = verify_partition_integrity(graph);

    printf("\n--- Final Statistics ---\n");
    printf("Actual cut edges: %d\n", actual_cut_edges);
    printf("Partition integrity: %s\n", partition_integrity ? "VALID" : "INVALID");

    // Statystyki rozkładu wierzchołków
    int *part_sizes = calloc(graph->parts, sizeof(int));
    if (!part_sizes)
        return;

    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id >= 0 && graph->nodes[i].part_id < graph->parts)
            part_sizes[graph->nodes[i].part_id]++;
    }

    printf("Vertex distribution across partitions:\n");
    for (int i = 0; i < graph->parts; i++)
    {
        float percentage = 100.0f * part_sizes[i] / graph->vertices;
        printf("  Partition %d: %d vertices (%.2f%%)\n", i, part_sizes[i], percentage);
    }

    free(part_sizes);
    printf("--- End of Statistics ---\n");
}

void print_partition_stats(FM_Context *context)
{
    printf("\n--- Partition Statistics ---\n");
    int total_vertices = 0;
    for (int i = 0; i < context->graph->parts; i++)
    {
        printf("  Part %d: %d vertices\n", i, context->part_sizes[i]);
        total_vertices += context->part_sizes[i];
    }
    printf("  Total vertices: %d\n", total_vertices);

    int boundary_count = 0;
    for (int i = 0; i < context->graph->vertices; i++)
    {
        bool is_boundary = false;
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                is_boundary = true;
                break;
            }
        }
        if (is_boundary)
            boundary_count++;
    }
    printf("  Boundary vertices: %d\n", boundary_count);
    printf("  Initial cut: %d\n", context->initial_cut);
    printf("--- End of Statistics ---\n");
}

void analyze_moves(FM_Context *context, bool *is_boundary)
{
    int total_moves = 0;
    int valid_moves = 0;
    int positive_gain_moves = 0;
    int balance_violations = 0;

    printf("\n--- Move Analysis ---\n");

    for (int i = 0; i < context->graph->vertices; i++)
    {
        if (is_boundary[i])
        {
            for (int p = 0; p < context->graph->parts; p++)
            {
                if (p != context->graph->nodes[i].part_id)
                {
                    total_moves++;
                    int is_valid = is_valid_move(context, i, p);
                    int gain = calculate_gain(context, i, p);

                    if (!is_valid)
                    {
                        balance_violations++;
                    }
                    else
                    {
                        valid_moves++;
                        if (gain > 0)
                        {
                            positive_gain_moves++;
                            printf("  Vertex %d can move from part %d to part %d with gain %d\n",
                                   i, context->graph->nodes[i].part_id, p, gain);
                        }
                    }
                }
            }
        }
    }

    printf("  Total possible moves: %d\n", total_moves);
    printf("  Valid moves: %d\n", valid_moves);
    printf("  Balance violations: %d\n", balance_violations);
    printf("  Moves with positive gain: %d\n", positive_gain_moves);
    printf("--- End of Analysis ---\n");
}

void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    // Jeśli max_iterations jest ujemne, ustaw domyślną wartość
    if (max_iterations <= 0)
    {
        max_iterations = 100; // Domyślna wartość
        printf("Warning: max_iterations was set to %d, using default value: %d\n", max_iterations, 100);
    }

    FM_Context *context = initialize_fm_context(graph, partition_data, max_iterations);
    if (!context)
    {
        fprintf(stderr, "Failed to initialize FM context\n");
        return;
    }

    // Osobna tablica dla wierzchołków granicznych
    bool *is_boundary = malloc(graph->vertices * sizeof(bool));
    if (!is_boundary)
    {
        fprintf(stderr, "Failed to allocate memory for boundary vertices\n");
        free_fm_context(context);
        return;
    }

    context->initial_cut = calculate_initial_cut(context);
    context->current_cut = context->initial_cut;

    // Wypisz statystyki partycji przed optymalizacją
    print_partition_stats(context);

    // Zidentyfikuj wierzchołki graniczne
    identify_boundary_vertices(context, is_boundary);

    // Przeanalizuj możliwe ruchy
    analyze_moves(context, is_boundary);

    // Jeśli początkowa liczba krawędzi przecinających jest 0, to nie ma co optymalizować
    if (context->initial_cut == 0)
    {
        printf("No crossing edges to optimize. Exiting.\n");
        free(is_boundary);
        free_fm_context(context);
        return;
    }

    printf("\nStarting FM optimization with %d max iterations\n", max_iterations);

    for (int iter = 0; iter < max_iterations; iter++)
    {
        // Zidentyfikuj wierzchołki graniczne
        identify_boundary_vertices(context, is_boundary);

        // Resetuj blokadę wierzchołków w każdej iteracji
        memset(context->locked, 0, context->graph->vertices * sizeof(bool));

        printf("Iteration %d: ", iter);

        // Znajdź najlepszy ruch
        int best_move_vertex = find_best_move(context, is_boundary);

        // Jeśli nie ma dostępnych ruchów, kończymy
        if (best_move_vertex == -1)
        {
            printf("No valid moves found. Stopping.\n");
            break;
        }

        int target_part = context->target_parts[best_move_vertex];

        // Zastosuj ruch tylko jeśli nie naruszy spójności
        if (!apply_move_safely(context, best_move_vertex, target_part))
        {
            // Jeśli ruch nie mógł być wykonany z powodu spójności, oznaczamy wierzchołek jako zablokowany
            context->locked[best_move_vertex] = true;
            continue; // Przejdź do następnej iteracji
        }

        printf("Current cut: %d\n", context->current_cut);
    }

    print_cut_statistics(context);

    // Po zakończeniu optymalizacji, dodaj weryfikację i statystyki
    print_final_statistics(graph);

    // Zwolnij pamięć
    free(is_boundary);
    free_fm_context(context);
}

// Modyfikacja find_best_move aby rozważyć również wierzchołki, które nie są graniczne
/**
 * Sprawdza, czy ruch wierzchołka jest dozwolony z zachowaniem integralności partycji
 * @param context Wskaźnik do kontekstu FM
 * @param vertex Indeks wierzchołka do przeniesienia
 * @param target_part Docelowa partycja
 * @return 1 jeśli ruch jest dozwolony, 0 w przeciwnym przypadku
 */
int is_move_valid_with_integrity(FM_Context *context, int vertex, int target_part)
{
    if (!context || !context->graph || vertex < 0 || vertex >= context->graph->vertices)
        return 0;

    // Najpierw sprawdź standardowe warunki
    if (!is_valid_move(context, vertex, target_part))
        return 0;

    // Następnie sprawdź, czy usunięcie wierzchołka nie naruszy spójności partycji źródłowej
    return will_remain_connected_if_removed(context->graph, vertex);
}

/**
 * Zastosuj ruch z zachowaniem spójności partycji
 * @param context Wskaźnik do kontekstu FM
 * @param vertex Indeks wierzchołka do przeniesienia
 * @param target_part Docelowa partycja
 * @return 1 jeśli ruch został wykonany, 0 w przeciwnym przypadku
 */
int apply_move_safely(FM_Context *context, int vertex, int target_part)
{
    if (!is_move_valid_with_integrity(context, vertex, target_part))
    {
        printf("WARNING: Move of vertex %d to part %d would break connectivity - skipping\n",
               vertex, target_part);
        return 0;
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // Zastosuj ruch
    context->graph->nodes[vertex].part_id = target_part;
    context->part_sizes[current_part]--;
    context->part_sizes[target_part]++;
    context->current_cut -= gain;
    context->moves_made++;
    context->locked[vertex] = true;

    printf("Move successful: vertex %d from part %d to part %d with gain %d\n",
           vertex, current_part, target_part, gain);

    return 1;
}

FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    if (!graph || !partition_data)
    {
        fprintf(stderr, "NULL input parameters\n");
        return NULL;
    }

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

    // Alokuj pamięć dla zablokowanych wierzchołków
    context->locked = malloc(graph->vertices * sizeof(bool));
    if (!context->locked)
    {
        fprintf(stderr, "Failed to allocate memory for locked vertices\n");
        free(context);
        return NULL;
    }
    memset(context->locked, 0, graph->vertices * sizeof(bool));

    // Alokuj pamięć na zyski
    context->gains = malloc(graph->vertices * sizeof(int));
    if (!context->gains)
    {
        fprintf(stderr, "Failed to allocate memory for gains\n");
        free(context->locked);
        free(context);
        return NULL;
    }

    // Alokuj pamięć na docelowe partycje
    context->target_parts = malloc(graph->vertices * sizeof(int));
    if (!context->target_parts)
    {
        fprintf(stderr, "Failed to allocate memory for target parts\n");
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }

    // Alokuj pamięć na rozmiary partycji
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

    // Inicjalizuj rozmiary partycji
    for (int i = 0; i < graph->parts; i++)
    {
        context->part_sizes[i] = partition_data->parts[i].part_vertex_count;
    }

    // Nie potrzebujemy już best_partition, bo nie będziemy zapisywać historii
    context->best_partition = NULL;

    // Inicjalizuj zyski i docelowe partycje
    for (int i = 0; i < graph->vertices; i++)
    {
        context->gains[i] = 0;
        context->target_parts[i] = graph->nodes[i].part_id;
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
        // Nie zwalniamy best_partition, bo nie jest alokowane
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
    return cut_edges / 2; // Każda krawędź liczona jest dwa razy
}

int calculate_gain(FM_Context *context, int vertex, int target_part)
{
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0; // Zabezpieczenie przed błędami segmentacji
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = 0;

    // Przejrzyj wszystkich sąsiadów wierzchołka
    for (int i = 0; i < context->graph->nodes[vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[vertex].neighbors[i];
        if (neighbor < 0 || neighbor >= context->graph->vertices)
        {
            continue; // Zabezpieczenie przed błędami segmentacji
        }

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
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0; // Nieprawidłowy wierzchołek lub partycja
    }

    int current_part = context->graph->nodes[vertex].part_id;

    if (context->locked[vertex])
    {
        return 0; // Wierzchołek jest już zablokowany (przesunięty w tej iteracji)
    }

    if (current_part == target_part)
    {
        return 0; // Wierzchołek jest już w docelowej partycji
    }

    // Sprawdź ograniczenia balansu
    int new_size_source = context->part_sizes[current_part] - 1;
    int new_size_target = context->part_sizes[target_part] + 1;
    int min_size = context->graph->min_count;
    int max_size = context->graph->max_count;

    if (new_size_source < min_size || new_size_target > max_size)
    {
        return 0; // Naruszyłoby to ograniczenia balansu
    }

    return 1; // Ruch jest dozwolony
}

void apply_move(FM_Context *context, int vertex, int target_part)
{
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return; // Zabezpieczenie przed błędami segmentacji
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // Aktualizuj przypisanie partycji
    context->graph->nodes[vertex].part_id = target_part;

    // Aktualizuj rozmiary partycji
    context->part_sizes[current_part]--;
    context->part_sizes[target_part]++;

    // Aktualizuj liczbę krawędzi przecinających i statystyki ruchów
    context->current_cut -= gain; // Odejmij zysk (ujemny zysk oznacza zwiększoną liczbę krawędzi)
    context->moves_made++;
    context->locked[vertex] = true; // Zablokuj wierzchołek po przesunięciu
}

void print_cut_statistics(FM_Context *context)
{
    printf("Initial cut: %d\n", context->initial_cut);
    printf("Current cut: %d\n", context->current_cut);
    printf("Best cut: %d\n", context->current_cut); // Teraz current_cut jest najlepszym
    printf("Moves made: %d\n", context->moves_made);
}

void check_partition_connectivity_fm(Graph *graph, int parts)
{
    printf("\n--- Checking partition connectivity ---\n");
    int all_connected = 1;

    for (int p = 0; p < parts; p++)
    {
        // Policz wierzchołki w partycji
        int count = 0;
        for (int i = 0; i < graph->vertices; i++)
            if (graph->nodes[i].part_id == p)
                count++;

        if (count == 0)
        {
            printf("Partition %d is empty - skipping\n", p);
            continue;
        }

        // Tablica odwiedzonych wierzchołków
        bool *visited = calloc(graph->vertices, sizeof(bool));
        int *queue = malloc(graph->vertices * sizeof(int));

        if (!visited || !queue)
        {
            printf("Failed to allocate memory for connectivity check\n");
            if (visited)
                free(visited);
            if (queue)
                free(queue);
            return;
        }

        // Znajdź pierwszy wierzchołek w partycji
        int start = -1;
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == p)
            {
                start = i;
                break;
            }
        }

        // BFS
        int front = 0, rear = 0;
        queue[rear++] = start;
        visited[start] = true;
        int visited_count = 1;

        while (front < rear)
        {
            int current = queue[front++];

            for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
            {
                int neighbor = graph->nodes[current].neighbors[i];
                if (graph->nodes[neighbor].part_id == p && !visited[neighbor])
                {
                    visited[neighbor] = true;
                    queue[rear++] = neighbor;
                    visited_count++;
                }
            }
        }

        int is_connected = (visited_count == count);
        printf("Partition %d: %s (visited %d/%d vertices)\n",
               p, is_connected ? "CONNECTED" : "DISCONNECTED", visited_count, count);

        if (!is_connected)
            all_connected = 0;

        free(visited);
        free(queue);
    }

    printf("Overall partition connectivity: %s\n", all_connected ? "VALID" : "INVALID");
    printf("--- End of connectivity check ---\n");
}