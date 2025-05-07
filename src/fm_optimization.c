#include "fm_optimization.h"
#include "graph.h"
#include "partition.h"
#include <pthread.h>
#include <unistd.h>

// struktura pomocnicza do przechowywania danych dla watku
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

// funkcja wykonywana przez watek, szuka najlepszego ruchu w przydzielonym zakresie wierzcholkow
void *thread_find_best_move(void *arg)
{
    // rzutuje argument na wlasciwy typ
    ThreadFindMoveData *data = (ThreadFindMoveData *)arg;
    FM_Context *context = data->context;
    bool *is_boundary = data->is_boundary;

    // inicjalizacja wartosci poczatkowych
    data->best_vertex = -1;
    data->best_gain = 0;

    // przegladam wierzcholki w przydzielonym zakresie
    for (int i = data->start_vertex; i < data->end_vertex; i++)
    {
        // sprawdzam tylko wierzcholki graniczne i niezablokowane
        if (is_boundary[i] && !context->locked[i])
        {
            // sprawdzam wszystkie mozliwe partie docelowe
            for (int p = 0; p < context->graph->parts; p++)
            {
                // omijam partie, w ktorej wierzcholek juz jest
                if (p != context->graph->nodes[i].part_id)
                {
                    // obliczam zysk z przeniesienia
                    int gain = calculate_gain(context, i, p);
                    // jesli zysk jest dodatni i najlepszy jak dotad i ruch jest dozwolony
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

// szuka najlepszego ruchu korzystajac z watkow do przyspieszenia
int find_best_move(FM_Context *context, bool *is_boundary)
{
    // okreslam optymalna liczbe watkow dla procesora
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[num_threads];
    ThreadFindMoveData thread_data[num_threads];

    // dziele wierzcholki miedzy watki
    int vertices_per_thread = context->graph->vertices / num_threads;
    int remaining = context->graph->vertices % num_threads;

    // tworze watki i przydzielam im zakresy wierzcholkow
    for (int t = 0; t < num_threads; t++)
    {
        thread_data[t].context = context;
        thread_data[t].is_boundary = is_boundary;
        thread_data[t].start_vertex = t * vertices_per_thread;
        thread_data[t].end_vertex = (t + 1) * vertices_per_thread;

        // ostatniemu watkowi daje pozostale wierzcholki
        if (t == num_threads - 1)
            thread_data[t].end_vertex += remaining;

        // tworze watek, jesli sie nie uda to przechodzę na wersje sekwencyjna
        if (pthread_create(&threads[t], NULL, thread_find_best_move, &thread_data[t]) != 0)
        {
            fprintf(stderr, "Error creating thread %d\n", t);
            // anuluje juz utworzone watki
            for (int i = 0; i < t; i++)
            {
                pthread_cancel(threads[i]);
            }

            // wykonuje sekwencyjna wersje find_best_move
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

    // czekam na zakonczenie wszystkich watkow
    for (int t = 0; t < num_threads; t++)
    {
        pthread_join(threads[t], NULL);
    }

    // wybieram najlepszy wynik z wszystkich watkow
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

    // zapisuje docelowa partie dla najlepszego wierzcholka
    if (best_vertex != -1)
    {
        context->target_parts[best_vertex] = best_target_part;
    }

    return best_vertex;
}

// sprawdza czy partycja jest spojna (wszystkie wierzcholki sa polaczone)
int is_partition_connected(Graph *graph, int part_id)
{
    // sprawdzam poprawnosc parametrow
    if (!graph || part_id < 0 || part_id >= graph->parts)
        return 0;

    // licze ile wierzcholkow jest w danej partycji
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
        if (graph->nodes[i].part_id == part_id)
            vertices_in_part++;

    // pusta partycja jest spojna z definicji
    if (vertices_in_part == 0)
        return 1;

    // tablice do przechodzenia grafu (BFS)
    bool *visited = calloc(graph->vertices, sizeof(bool));
    if (!visited)
        return 0;

    int *queue = malloc(graph->vertices * sizeof(int));
    if (!queue)
    {
        free(visited);
        return 0;
    }

    // szukam pierwszego wierzcholka w partycji
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id)
        {
            start_vertex = i;
            break;
        }
    }

    // inicjalizuje kolejke do BFS
    int front = 0, rear = 0;
    queue[rear++] = start_vertex;
    visited[start_vertex] = true;
    int nodes_visited = 1;

    // przechodzę grafem metoda BFS
    while (front < rear)
    {
        int current = queue[front++];

        for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
        {
            int neighbor = graph->nodes[current].neighbors[i];

            // dodaje do kolejki tylko sasiadow z tej samej partycji
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

    // partycja jest spojna jesli odwiedzilem wszystkie wierzcholki
    return (nodes_visited == vertices_in_part);
}

// sprawdza czy usuniecie wierzcholka z partycji nie naruszy jej spojnosci
int will_remain_connected_if_removed(Graph *graph, int vertex)
{
    // wypisuje info diagnostyczne
    printf("Checking if removing vertex %d will preserve connectivity...\n", vertex);

    int current_part = graph->nodes[vertex].part_id;

    // licze wierzcholki w partycji (bez przenoszonego wierzcholka)
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (i != vertex && graph->nodes[i].part_id == current_part)
            vertices_in_part++;
    }

    printf("Part %d has %d vertices (excluding vertex %d)\n",
           current_part, vertices_in_part, vertex);

    // jesli zostaje co najwyzej 1 wierzcholek, to partycja bedzie spojna
    if (vertices_in_part <= 1)
    {
        printf("Part has <= 1 vertex, so it remains connected\n");
        return 1;
    }

    // zapamietuje oryginalna partie i tymczasowo "usuwam" wierzcholek
    int original_part = graph->nodes[vertex].part_id;
    graph->nodes[vertex].part_id = -1;

    // przygotowuje tablice do BFS
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *queue = malloc(graph->vertices * sizeof(int));

    if (!visited || !queue)
    {
        if (visited)
            free(visited);
        if (queue)
            free(queue);
        graph->nodes[vertex].part_id = original_part; // przywracam oryginalna partie
        return 0;
    }

    // szukam pierwszego wierzcholka w partycji po "usunieciu"
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == original_part)
        {
            start_vertex = i;
            break;
        }
    }

    // BFS zeby sprawdzic spojnosc po usunieciu
    int front = 0, rear = 0;
    queue[rear++] = start_vertex;
    visited[start_vertex] = true;
    int nodes_visited = 1;

    while (front < rear)
    {
        int current = queue[front++];

        // sprawdzam sasiadow
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

    // przywracam oryginalna partie wierzcholka
    graph->nodes[vertex].part_id = original_part;

    // sprawdzam czy wszystkie wierzcholki zostaly odwiedzone
    int result = (nodes_visited == vertices_in_part);

    free(visited);
    free(queue);

    return result;
}

// weryfikuje integralnosc wszystkich partycji w grafie
int verify_partition_integrity(Graph *graph)
{
    if (!graph)
        return 0;

    int all_connected = 1;

    printf("\n--- Partition Integrity Verification ---\n");
    for (int i = 0; i < graph->parts; i++)
    {
        // sprawdzam spojnosc kazdej partycji
        int is_connected = is_partition_connected(graph, i);
        printf("Partition %d is %s\n", i, is_connected ? "connected" : "DISCONNECTED");

        if (!is_connected)
            all_connected = 0;
    }

    printf("Overall partition integrity: %s\n", all_connected ? "VALID" : "INVALID");
    printf("--- End of Verification ---\n");

    return all_connected;
}

// liczy rzeczywista liczbe przecietych krawedzi w grafie
int count_cut_edges(Graph *graph)
{
    if (!graph)
        return 0;

    int cut_edges = 0;

    // przegladam wszystkie krawedzie
    for (int i = 0; i < graph->vertices; i++)
    {
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = graph->nodes[i].neighbors[j];

            // licze tylko w jedna strone, zeby nie liczyc podwojnie
            if (i < neighbor && graph->nodes[i].part_id != graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }

    return cut_edges;
}

// wypisuje statystyki po zakonczeniu optymalizacji
void print_final_statistics(Graph *graph)
{
    if (!graph)
        return;

    // obliczam faktyczna liczbe przecietych krawedzi
    int actual_cut_edges = count_cut_edges(graph);
    int partition_integrity = verify_partition_integrity(graph);

    printf("\n--- Final Statistics ---\n");
    printf("Actual cut edges: %d\n", actual_cut_edges);
    printf("Partition integrity: %s\n", partition_integrity ? "VALID" : "INVALID");

    // statystyki rozkladu wierzcholkow miedzy partie
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

// wypisuje statystyki partycji
void print_partition_stats(FM_Context *context)
{
    printf("\n--- Partition Statistics ---\n");

    // zliczam wierzcholki w kazdej partycji
    int total_vertices = 0;
    for (int i = 0; i < context->graph->parts; i++)
    {
        printf("  Part %d: %d vertices\n", i, context->part_sizes[i]);
        total_vertices += context->part_sizes[i];
    }
    printf("  Total vertices: %d\n", total_vertices);

    // zliczam wierzcholki graniczne
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

// analizuje dostepne ruchy w aktualnym stanie grafu
void analyze_moves(FM_Context *context, bool *is_boundary)
{
    int total_moves = 0;
    int valid_moves = 0;
    int positive_gain_moves = 0;
    int balance_violations = 0;

    printf("\n--- Move Analysis ---\n");

    // dla kazdego wierzcholka analizuje mozliwe ruchy
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

    // podsumowanie analizy
    printf("  Total possible moves: %d\n", total_moves);
    printf("  Valid moves: %d\n", valid_moves);
    printf("  Balance violations: %d\n", balance_violations);
    printf("  Moves with positive gain: %d\n", positive_gain_moves);
    printf("--- End of Analysis ---\n");
}

// glowna funkcja algorytmu optymalizacji Fiduccia-Mattheysa
void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    // ustawiam domyslna liczbe iteracji jesli podana jest niepoprawna
    if (max_iterations <= 0)
    {
        max_iterations = 100;
        printf("Warning: max_iterations was set to %d, using default value: %d\n", max_iterations, 100);
    }

    // inicjalizuje kontekst algorytmu FM
    FM_Context *context = initialize_fm_context(graph, partition_data, max_iterations);
    if (!context)
    {
        fprintf(stderr, "Failed to initialize FM context\n");
        return;
    }

    // alokuje pamiec na tablice wierzcholkow granicznych
    bool *is_boundary = malloc(graph->vertices * sizeof(bool));
    if (!is_boundary)
    {
        fprintf(stderr, "Failed to allocate memory for boundary vertices\n");
        free_fm_context(context);
        return;
    }

    // obliczam poczatkowa liczbe przecietych krawedzi
    context->initial_cut = calculate_initial_cut(context);
    context->current_cut = context->initial_cut;

    // wypisuje statystyki przed optymalizacja
    print_partition_stats(context);

    // znajduje wierzcholki graniczne
    identify_boundary_vertices(context, is_boundary);

    // analizuje mozliwe ruchy
    analyze_moves(context, is_boundary);

    // jesli nie ma krawedzi do optymalizacji, koncze
    if (context->initial_cut == 0)
    {
        printf("No crossing edges to optimize. Exiting.\n");
        free(is_boundary);
        free_fm_context(context);
        return;
    }

    printf("\nStarting FM optimization with %d max iterations\n", max_iterations);

    // glowna petla algorytmu
    for (int iter = 0; iter < max_iterations; iter++)
    {
        // aktualizuje liste wierzcholkow granicznych
        identify_boundary_vertices(context, is_boundary);

        // odblokowuje wszystkie wierzcholki w kazdej iteracji
        memset(context->locked, 0, context->graph->vertices * sizeof(bool));

        printf("Iteration %d: ", iter);

        // szukam najlepszego ruchu
        int best_move_vertex = find_best_move(context, is_boundary);

        // koniec jesli nie ma dostepnych ruchow
        if (best_move_vertex == -1)
        {
            printf("No valid moves found. Stopping.\n");
            break;
        }

        int target_part = context->target_parts[best_move_vertex];

        // wykonuje ruch tylko jesli nie naruszy spojnosci
        if (!apply_move_safely(context, best_move_vertex, target_part))
        {
            // jesli ruch niemozliwy, blokuje wierzcholek
            context->locked[best_move_vertex] = true;
            continue;
        }

        printf("Current cut: %d\n", context->current_cut);
    }

    // wyswietlam podsumowanie
    print_cut_statistics(context);

    // wyswietlam dodatkowe statystyki i weryfikacje
    print_final_statistics(graph);

    // zwalniam pamiec
    free(is_boundary);
    free_fm_context(context);
}

// sprawdza czy ruch jest dozwolony z zachowaniem spojnosci
int is_move_valid_with_integrity(FM_Context *context, int vertex, int target_part)
{
    if (!context || !context->graph || vertex < 0 || vertex >= context->graph->vertices)
        return 0;

    // sprawdzam standardowe ograniczenia
    if (!is_valid_move(context, vertex, target_part))
        return 0;

    // sprawdzam czy usuniecie wierzcholka nie popsuje spojnosci
    return will_remain_connected_if_removed(context->graph, vertex);
}

// wykonuje ruch z zachowaniem spojnosci partycji
int apply_move_safely(FM_Context *context, int vertex, int target_part)
{
    // sprawdzam czy ruch jest dozwolony
    if (!is_move_valid_with_integrity(context, vertex, target_part))
    {
        printf("WARNING: Move of vertex %d to part %d would break connectivity - skipping\n",
               vertex, target_part);
        return 0;
    }

    // zapamietuje obecna partie i obliczam zysk
    int current_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // wykonuje ruch
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

// inicjalizuje strukture kontekstu dla algorytmu FM
FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    // sprawdzam poprawnosc parametrow
    if (!graph || !partition_data)
    {
        fprintf(stderr, "NULL input parameters\n");
        return NULL;
    }

    // alokuje pamiec na kontekst
    FM_Context *context = malloc(sizeof(FM_Context));
    if (!context)
    {
        fprintf(stderr, "Failed to allocate memory for FM context\n");
        return NULL;
    }

    // ustawiam podstawowe pola
    context->graph = graph;
    context->partition = partition_data;
    context->max_iterations = max_iterations;
    context->iterations = 0;
    context->moves_made = 0;
    context->initial_cut = 0;
    context->current_cut = 0;
    context->best_cut = 0;

    // alokuje pamiec na pomocnicze tablice
    context->locked = malloc(graph->vertices * sizeof(bool));
    if (!context->locked)
    {
        fprintf(stderr, "Failed to allocate memory for locked vertices\n");
        free(context);
        return NULL;
    }
    memset(context->locked, 0, graph->vertices * sizeof(bool));

    // alokuje pamiec na zyski
    context->gains = malloc(graph->vertices * sizeof(int));
    if (!context->gains)
    {
        fprintf(stderr, "Failed to allocate memory for gains\n");
        free(context->locked);
        free(context);
        return NULL;
    }

    // alokuje pamiec na docelowe partie
    context->target_parts = malloc(graph->vertices * sizeof(int));
    if (!context->target_parts)
    {
        fprintf(stderr, "Failed to allocate memory for target parts\n");
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }

    // alokuje pamiec na rozmiary partycji
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

    // inicjalizuje rozmiary partycji
    for (int i = 0; i < graph->parts; i++)
    {
        context->part_sizes[i] = partition_data->parts[i].part_vertex_count;
    }

    // nie potrzebuje best_partition
    context->best_partition = NULL;

    // inicjalizuje tablice zyskow i docelowych partycji
    for (int i = 0; i < graph->vertices; i++)
    {
        context->gains[i] = 0;
        context->target_parts[i] = graph->nodes[i].part_id;
    }

    return context;
}

// zwalnia pamiec zaalokowana dla kontekstu FM
void free_fm_context(FM_Context *context)
{
    if (context)
    {
        free(context->locked);
        free(context->gains);
        free(context->target_parts);
        free(context->part_sizes);
        free(context);
    }
}

// znajduje wierzcholki graniczne (te, ktore maja sasiadow w innych partycjach)
void identify_boundary_vertices(FM_Context *context, bool *is_boundary)
{
    for (int i = 0; i < context->graph->vertices; i++)
    {
        // najpierw zakladam, ze wierzcholek nie jest graniczny
        is_boundary[i] = false;

        // sprawdzam wszystkich sasiadow
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            // jesli sasiad jest w innej partycji, to wierzcholek jest graniczny
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                is_boundary[i] = true;
                break;
            }
        }
    }
}

// oblicza poczatkowa liczbe przecietych krawedzi
int calculate_initial_cut(FM_Context *context)
{
    int cut_edges = 0;

    // przegladam wszystkie krawedzie
    for (int i = 0; i < context->graph->vertices; i++)
    {
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            // jesli sasiad jest w innej partycji, to krawedz jest przecieta
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }
    // dziele przez 2, bo kazda krawedz jest liczona dwukrotnie
    return cut_edges / 2;
}

// oblicza zysk z przeniesienia wierzcholka do innej partycji
int calculate_gain(FM_Context *context, int vertex, int target_part)
{
    // sprawdzam poprawnosc parametrow
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0;
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = 0;

    // przegladam sasiadow wierzcholka
    for (int i = 0; i < context->graph->nodes[vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[vertex].neighbors[i];
        if (neighbor < 0 || neighbor >= context->graph->vertices)
        {
            continue;
        }

        int neighbor_part = context->graph->nodes[neighbor].part_id;

        // sasiad w docelowej partycji - zysk bo krawedz nie bedzie juz przecieta
        if (neighbor_part == target_part)
        {
            gain++;
        }
        // sasiad w aktualnej partycji - strata bo krawedz stanie sie przecieta
        else if (neighbor_part == current_part)
        {
            gain--;
        }
        // sasiady w innych partycjach nie wplywaja na zmiane
    }

    return gain;
}

// sprawdza czy dany ruch jest dozwolony
int is_valid_move(FM_Context *context, int vertex, int target_part)
{
    // sprawdzam poprawnosc parametrow
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0;
    }

    int current_part = context->graph->nodes[vertex].part_id;

    // nie przenosze zabloklowanych wierzcholkow
    if (context->locked[vertex])
    {
        return 0;
    }

    // nie przenosze do tej samej partycji
    if (current_part == target_part)
    {
        return 0;
    }

    // sprawdzam ograniczenia rozmiaru partycji
    int new_size_source = context->part_sizes[current_part] - 1;
    int new_size_target = context->part_sizes[target_part] + 1;
    int min_size = context->graph->min_count;
    int max_size = context->graph->max_count;

    // jesli narusza ograniczenia, to ruch jest niedozwolony
    if (new_size_source < min_size || new_size_target > max_size)
    {
        return 0;
    }

    return 1;
}

// wykonuje ruch wierzcholka do innej partycji
void apply_move(FM_Context *context, int vertex, int target_part)
{
    // sprawdzam poprawnosc parametrow
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return;
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // zmieniam przypisanie partycji
    context->graph->nodes[vertex].part_id = target_part;

    // aktualizuje rozmiary partycji
    context->part_sizes[current_part]--;
    context->part_sizes[target_part]++;

    // aktualizuje liczbe krawedzi przecinajacych
    context->current_cut -= gain;
    context->moves_made++;
    context->locked[vertex] = true;
}

// wypisuje statystyki optymalizacji
void print_cut_statistics(FM_Context *context)
{
    printf("Initial cut: %d\n", context->initial_cut);
    printf("Current cut: %d\n", context->current_cut);
    printf("Best cut: %d\n", context->current_cut);
    printf("Moves made: %d\n", context->moves_made);
}
