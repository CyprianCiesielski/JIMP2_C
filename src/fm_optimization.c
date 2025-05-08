#include "fm_optimization.h"

// struktura dla danych watku, stara implementacja, juz nie uzywana
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

// stara funkcja dla watkow, juz nie uzywana - zostawiona dla kompatybilnosci wstecznej
void *thread_find_best_move(void *arg)
{
    // bierzemy dane z argumentu
    ThreadFindMoveData *data = (ThreadFindMoveData *)arg;
    FM_Context *context = data->context;
    bool *is_boundary = data->is_boundary;

    // na poczatku nic nie znalezlismy
    data->best_vertex = -1;
    data->best_gain = 0;

    // sprawdzamy wszystkie wierzcholki w zakresie
    for (int i = data->start_vertex; i < data->end_vertex; i++)
    {
        // tylko wierzcholki graniczne i niezablokowane
        if (is_boundary[i] && !context->locked[i])
        {
            // probujemy wszystkie partie docelowe
            for (int p = 0; p < context->graph->parts; p++)
            {
                // omijamy partie w ktorej juz jest wierzcholek
                if (p != context->graph->nodes[i].part_id)
                {
                    // sprawdzamy zysk
                    int gain = calculate_gain(context, i, p);
                    // jesli jest lepszy niz poprzedni i ruch jest ok
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

// znajduje najlepszy mozliwy ruch w grafie
int find_best_move(FM_Context *context, bool *is_boundary)
{
    int best_vertex = -1;
    int best_gain = 0;
    int best_target_part = -1;

    // sprawdzamy po kolei wszystkie wierzcholki
    for (int i = 0; i < context->graph->vertices; i++)
    {
        // tylko graniczne, niezablokowane i nie zabanowane
        if (is_boundary[i] && !context->locked[i] && !context->unmovable[i])
        {
            // sprawdzamy wszystkie mozliwe partie docelowe
            for (int p = 0; p < context->graph->parts; p++)
            {
                // omijamy partie w ktorej wierzcholek juz jest
                if (p != context->graph->nodes[i].part_id)
                {
                    int gain = calculate_gain(context, i, p);
                    // ruch musi byc zyskowny i nie psuc spojnosci
                    if (gain > 0 && gain > best_gain && is_move_valid_with_integrity(context, i, p))
                    {
                        best_gain = gain;
                        best_vertex = i;
                        best_target_part = p;
                    }
                }
            }
        }
    }

    // jesli znalezlismy dobry ruch, zapisujemy partie docelowa
    if (best_vertex != -1)
    {
        context->target_parts[best_vertex] = best_target_part;
    }

    return best_vertex;
}

// sprawdza czy cala partycja jest spojna
int is_partition_connected(Graph *graph, int part_id)
{
    // sprawdzamy parametry
    if (!graph)
        return 0;

    // liczymy ile wierzcholkow jest w tej partycji
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
        if (graph->nodes[i].part_id == part_id)
            vertices_in_part++;

    // pusta partycja jest ok
    if (vertices_in_part == 0)
        return 1;

    // jeden wierzcholek tez jest ok
    if (vertices_in_part == 1)
        return 1;

    // alokujemy pamiec na odwiedzone i kolejke
    bool *visited = calloc(graph->vertices, sizeof(bool));
    if (!visited)
        return 0;

    int *queue = malloc(graph->vertices * sizeof(int));
    if (!queue)
    {
        free(visited);
        return 0;
    }

    // szukamy pierwszego wierzcholka w partycji
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id)
        {
            start_vertex = i;
            break;
        }
    }

    // robimy BFS zaczynajac od znalezionego wierzcholka
    int front = 0, rear = 0;
    queue[rear++] = start_vertex;
    visited[start_vertex] = true;
    int nodes_visited = 1;

    while (front < rear)
    {
        int current = queue[front++];

        // sprawdzamy sasiadow obecnego wierzcholka
        for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
        {
            int neighbor = graph->nodes[current].neighbors[i];

            // dodajemy do kolejki tylko sasiadow z tej samej partycji
            if (graph->nodes[neighbor].part_id == part_id && !visited[neighbor])
            {
                visited[neighbor] = true;
                queue[rear++] = neighbor;
                nodes_visited++;
            }
        }
    }

    // sprzatamy
    free(queue);
    free(visited);

    // partycja jest spojna jesli odwiedzilismy wszystkie wierzcholki
    return (nodes_visited == vertices_in_part);
}

// sprawdza czy partycja pozostanie spojna jesli usuniemy z niej wierzcholek
int will_remain_connected_if_removed(Graph *graph, int vertex)
{
    int current_part = graph->nodes[vertex].part_id;

    // liczymy wierzcholki w partycji bez usuwanego
    int vertices_in_part = 0;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (i != vertex && graph->nodes[i].part_id == current_part)
            vertices_in_part++;
    }

    // jesli zostaje 0 lub 1 wierzcholek, to bedzie spojna
    if (vertices_in_part <= 1)
    {
        // printf("DEBUG: Moving vertex %d - source partition will have %d vertices - SPOJNOSC OK\n",vertex, vertices_in_part);
        return 1;
    }

    // alokujemy pamiec na BFS
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *queue = malloc(graph->vertices * sizeof(int));
    if (!visited || !queue)
    {
        if (visited)
            free(visited);
        if (queue)
            free(queue);
        return 0;
    }

    // szukamy pierwszego wierzcholka w partycji (innego niz usuwany)
    int start_vertex = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (i != vertex && graph->nodes[i].part_id == current_part)
        {
            start_vertex = i;
            break;
        }
    }

    // robimy BFS omijajac usuwany wierzcholek
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

            // pomijamy usuwany wierzcholek
            if (neighbor != vertex &&
                graph->nodes[neighbor].part_id == current_part &&
                !visited[neighbor])
            {
                visited[neighbor] = true;
                queue[rear++] = neighbor;
                nodes_visited++;
            }
        }
    }

    // sprawdzamy czy wszystkie wierzcholki sa osiagalne
    int result = (nodes_visited == vertices_in_part);

    free(visited);
    free(queue);

    return result;
}

// weryfikuje spojnosc wszystkich partycji w grafie
int verify_partition_integrity(Graph *graph)
{
    if (!graph)
        return 0;

    int all_connected = 1;

    // szukamy wszystkich unikalnych id partycji
    int *unique_partitions = calloc(graph->vertices, sizeof(int));
    bool *found = calloc(graph->vertices, sizeof(bool));
    int unique_count = 0;

    if (!unique_partitions || !found)
    {
        free(unique_partitions);
        free(found);
        return 0;
    }

    // zbieramy wszystkie partie
    for (int i = 0; i < graph->vertices; i++)
    {
        int part_id = graph->nodes[i].part_id;
        if (!found[part_id])
        {
            found[part_id] = true;
            unique_partitions[unique_count++] = part_id;
        }
    }

    // wypisujemy status kazdej partycji
    // printf("\n--- Partition Integrity Verification ---\n");
    for (int i = 0; i < unique_count; i++)
    {
        int part_id = unique_partitions[i];
        int is_connected = is_partition_connected(graph, part_id);
        // printf("Partition %d is %s\n", part_id, is_connected ? "connected" : "DISCONNECTED");

        if (!is_connected)
            all_connected = 0;
    }

    free(unique_partitions);
    free(found);

    // podsumowanie
    // printf("Overall partition integrity: %s\n", all_connected ? "VALID" : "INVALID");
    // printf("--- End of Verification ---\n");

    return all_connected;
}

// liczy ile krawedzi przecina granice partycji
int count_cut_edges(Graph *graph)
{
    if (!graph)
        return 0;

    int cut_edges = 0;

    // przechodzimy przez wszystkie krawedzie
    for (int i = 0; i < graph->vertices; i++)
    {
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = graph->nodes[i].neighbors[j];

            // liczymy tylko w jedna strone, zeby nie liczyc podwojnie
            if (i < neighbor && graph->nodes[i].part_id != graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }

    return cut_edges;
}

// wypisuje koncowe statystyki partycji
void print_final_statistics(Graph *graph)
{
    if (!graph)
        return;

    // liczymy faktyczne przeciecia krawedzi
    int actual_cut_edges = count_cut_edges(graph);
    int partition_integrity = verify_partition_integrity(graph);

    // wypisujemy wyniki
    // printf("\n--- Final Statistics ---\n");
    // printf("Actual cut edges: %d\n", actual_cut_edges);
    // printf("Partition integrity: %s\n", partition_integrity ? "VALID" : "INVALID");

    // liczymy rozklad wierzcholkow
    int *part_sizes = calloc(graph->parts, sizeof(int));
    if (!part_sizes)
        return;

    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id >= 0 && graph->nodes[i].part_id < graph->parts)
            part_sizes[graph->nodes[i].part_id]++;
    }

    // wypisujemy rozklad
    // printf("Vertex distribution across partitions:\n");
    for (int i = 0; i < graph->parts; i++)
    {
        float percentage = 100.0f * part_sizes[i] / graph->vertices;
        // printf("  Partition %d: %d vertices (%.2f%%)\n", i, part_sizes[i], percentage);
    }

    free(part_sizes);
    printf("--- End of Statistics ---\n");
}

// wyswietla statystyki partycji
void print_partition_stats(FM_Context *context)
{
    printf("\n--- Partition Statistics ---\n");

    // liczymy wierzcholki w partiach
    int total_vertices = 0;
    for (int i = 0; i < context->graph->parts; i++)
    {
        // wypisuje ile wierzcholkow jest w danej partii oraz wypisuje ile procent sredniej ilosci wierzcholkow ma ta partia czyli np 105% z dokladnoscia do 2 miejsc po przecinku
        float percentage = 100.0f * context->part_sizes[i] / (context->graph->vertices / context->graph->parts);

        printf("  Partition %d: %d vertices (%.2f%%)\n", i, context->part_sizes[i], percentage);
        total_vertices += context->part_sizes[i];
    }
    printf("  Total vertices: %d\n", total_vertices);

    // liczymy wierzcholki na granicy
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

    // printf("  Boundary vertices: %d\n", boundary_count);
    // printf("  Initial cut: %d\n", context->initial_cut);
    // printf("--- End of Statistics ---\n");
}

// analizuje jakie ruchy sa dostepne
void analyze_moves(FM_Context *context, bool *is_boundary)
{
    int total_moves = 0;
    int valid_moves = 0;
    int positive_gain_moves = 0;
    int balance_violations = 0;

    // printf("\n--- Move Analysis ---\n");

    // sprawdzamy wszystkie wierzcholki
    for (int i = 0; i < context->graph->vertices; i++)
    {
        if (is_boundary[i])
        {
            // dla kazdej mozliwej partycji docelowej
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
                            // printf("  Vertex %d can move from part %d to part %d with gain %d\n",i, context->graph->nodes[i].part_id, p, gain);
                        }
                    }
                }
            }
        }
    }

    // wypisujemy podsumowanie
    // printf("  Total possible moves: %d\n", total_moves);
    // printf("  Valid moves: %d\n", valid_moves);
    // printf("  Balance violations: %d\n", balance_violations);
    // printf("  Moves with positive gain: %d\n", positive_gain_moves);
    // printf("--- End of Analysis ---\n");
}

// glowna funkcja algorytmu FM do optymalizacji ciec krawedzi
void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    // sprawdzamy czy liczba iteracji jest sensowna
    if (max_iterations <= 0)
    {
        max_iterations = 100;
        // printf("Warning: max_iterations was set to %d, using default value: %d\n", max_iterations, 100);
    }

    // inicjalizujemy kontekst algorytmu
    FM_Context *context = initialize_fm_context(graph, partition_data, max_iterations);
    if (!context)
    {
        fprintf(stderr, "Failed to initialize FM context\n");
        return;
    }

    // alokujemy pamiec na oznaczenie wierzcholkow granicznych
    bool *is_boundary = malloc(graph->vertices * sizeof(bool));
    if (!is_boundary)
    {
        fprintf(stderr, "Failed to allocate memory for boundary vertices\n");
        free_fm_context(context);
        return;
    }

    // obliczamy poczatkowy stan
    context->initial_cut = calculate_initial_cut(context);
    context->current_cut = context->initial_cut;

    // pokazujemy statystyki przed optymalizacja
    print_partition_stats(context);

    // znajdujemy wierzcholki na granicy partycji
    identify_boundary_vertices(context, is_boundary);

    // analizujemy mozliwe ruchy
    analyze_moves(context, is_boundary);

    // jesli nie ma co optymalizowac to konczymy
    if (context->initial_cut == 0)
    {
        // printf("No crossing edges to optimize. Exiting.\n");
        free(is_boundary);
        free_fm_context(context);
        return;
    }

    // printf("\nStarting FM optimization with %d max iterations\n", max_iterations);

    // glowna petla algorytmu
    for (int iter = 0; iter < max_iterations; iter++)
    {
        // aktualizacja wierzcholkow granicznych
        identify_boundary_vertices(context, is_boundary);

        // odblokowujemy wierzcholki na poczatku kazdej iteracji
        memset(context->locked, 0, context->graph->vertices * sizeof(bool));

        // printf("Iteration %d: ", iter);

        // szukamy najlepszego mozliwego ruchu
        int best_move_vertex = find_best_move(context, is_boundary);

        // jesli nie znalezlismy zadnego ruchu, konczymy
        if (best_move_vertex == -1)
        {
            // printf("No valid moves found. Stopping.\n");
            break;
        }

        int target_part = context->target_parts[best_move_vertex];

        // wykonujemy ruch tylko jesli nie psuje spojnosci
        if (!apply_move_safely(context, best_move_vertex, target_part))
        {
            // jesli nie mozemy wykonac ruchu, blokujemy wierzcholek
            context->locked[best_move_vertex] = true;
            // printf("Move not possible for vertex %d to part %d. Blocking vertex.\n",best_move_vertex, target_part);
            continue;
        }

        // sprawdzamy czy po ruchu nadal wszystko jest ok
        if (!verify_partition_integrity(context->graph))
        {
            // printf("CRITICAL ERROR: Partition integrity violated in iteration %d!\n", iter);
            //  przerywamy algorytm bo cos sie zepsulo
            break;
        }

        // printf("Current cut: %d\n", context->current_cut);
    }

    // pokazujemy wyniki
    print_cut_statistics(context);

    // dodatkowe statystyki
    // print_final_statistics(graph);

    // tu mozna by przywrocic najlepsza znaleziona partycje
    if (context->best_cut < context->initial_cut)
    {
        // printf("Restoring best partition with cut %d\n", context->best_cut);
        // restore_best_solution(context);
    }

    // zwalniamy pamiec
    free(is_boundary);
    free_fm_context(context);
}

// sprawdza czy ruch wierzcholka nie popsuje spojnosci partycji
int is_move_valid_with_integrity(FM_Context *context, int vertex, int target_part)
{
    // najpierw sprawdzamy podstawowe warunki
    if (!is_valid_move(context, vertex, target_part))
    {
        return 0;
    }

    // zapamietujemy obecna partycje
    int source_part = context->graph->nodes[vertex].part_id;

    // sprawdzamy czy po usunieciu wierzcholka partycja zostanie spojna
    if (!will_remain_connected_if_removed(context->graph, vertex))
    {
        // printf("DEBUG: Moving vertex %d would break source partition %d connectivity\n",vertex, source_part);
        context->unmovable[vertex] = true; // banujemy wierzcholek
        return 0;
    }

    // sprawdzamy czy wierzcholek ma polaczenie z nowa partycja
    int has_connection = 0;
    for (int i = 0; i < context->graph->nodes[vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[vertex].neighbors[i];
        if (context->graph->nodes[neighbor].part_id == target_part)
        {
            has_connection = 1;
            break;
        }
    }

    // nie mozna przeniesc jesli nie ma polaczenia
    if (!has_connection)
    {
        // printf("DEBUG: Vertex %d has no connection to target partition %d\n",vertex, target_part);
        context->unmovable[vertex] = true;
        return 0;
    }

    // wszystko ok
    return 1;
}

// bezpiecznie przenosi wierzcholek miedzy partycjami
int apply_move_safely(FM_Context *context, int vertex, int target_part)
{
    // jeszcze raz weryfikujemy poprawnosc ruchu
    if (!is_move_valid_with_integrity(context, vertex, target_part))
    {
        context->unmovable[vertex] = true;
        return 0;
    }

    // zapamietujemy stan poczatkowy
    int source_part = context->graph->nodes[vertex].part_id;
    int gain = calculate_gain(context, vertex, target_part);

    // printf("DEBUG: Moving vertex %d from part %d to part %d (gain: %d)\n",vertex, source_part, target_part, gain);

    // wykonujemy ruch
    context->graph->nodes[vertex].part_id = target_part;
    context->part_sizes[source_part]--;
    context->part_sizes[target_part]++;

    // sprawdzamy czy wszystkie partycje sa nadal spojne
    if (!verify_partition_integrity(context->graph))
    {
        // printf("CRITICAL ERROR: Move broke overall partition integrity!\n");

        // cofamy ruch
        context->graph->nodes[vertex].part_id = source_part;
        context->part_sizes[source_part]++;
        context->part_sizes[target_part]--;
        context->unmovable[vertex] = true; // banujemy na przyszlosc
        return 0;
    }

    // aktualizujemy stan po ruchu
    context->current_cut -= gain;
    context->moves_made++;
    context->locked[vertex] = true;

    // zapamietujemy najlepszy wynik
    if (context->current_cut < context->best_cut)
    {
        context->best_cut = context->current_cut;
    }

    return 1;
}

// inicjalizuje wszystkie struktury potrzebne dla algorytmu FM
FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations)
{
    // sprawdzamy parametry
    if (!graph || !partition_data)
    {
        fprintf(stderr, "NULL input parameters\n");
        return NULL;
    }

    // alokujemy pamiec na kontekst
    FM_Context *context = malloc(sizeof(FM_Context));
    if (!context)
    {
        fprintf(stderr, "Failed to allocate memory for FM context\n");
        return NULL;
    }

    // ustawiamy podstawowe wartosci
    context->graph = graph;
    context->partition = partition_data;
    context->max_iterations = max_iterations;
    context->iterations = 0;
    context->moves_made = 0;
    context->initial_cut = 0;
    context->current_cut = 0;
    context->best_cut = 10000000; // najlepszy wynik zaczynamy od duzej wartosci

    // alokujemy pamiec na tablice zablokowanych wierzcholkow
    context->locked = malloc(graph->vertices * sizeof(bool));
    if (!context->locked)
    {
        fprintf(stderr, "Failed to allocate memory for locked vertices\n");
        free(context);
        return NULL;
    }
    memset(context->locked, 0, graph->vertices * sizeof(bool));

    // alokujemy pamiec na tablice zyskow
    context->gains = malloc(graph->vertices * sizeof(int));
    if (!context->gains)
    {
        fprintf(stderr, "Failed to allocate memory for gains\n");
        free(context->locked);
        free(context);
        return NULL;
    }

    // alokujemy pamiec na docelowe partie
    context->target_parts = malloc(graph->vertices * sizeof(int));
    if (!context->target_parts)
    {
        fprintf(stderr, "Failed to allocate memory for target parts\n");
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }

    // alokujemy pamiec na rozmiary partycji
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

    // alokujemy pamiec na zabanowane wierzcholki
    context->unmovable = calloc(graph->vertices, sizeof(bool));
    if (!context->unmovable)
    {
        fprintf(stderr, "Failed to allocate memory for unmovable vertices\n");
        free(context->part_sizes);
        free(context->target_parts);
        free(context->gains);
        free(context->locked);
        free(context);
        return NULL;
    }

    // inicjalizujemy rozmiary partycji
    for (int i = 0; i < graph->parts; i++)
    {
        context->part_sizes[i] = partition_data->parts[i].part_vertex_count;
    }

    // na razie nie uzywamy best_partition
    context->best_partition = NULL;

    // inicjalizujemy tablice zyskow i docelowych partycji
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
        free(context->unmovable);
        free(context->gains);
        free(context->target_parts);
        free(context->part_sizes);
        free(context);
    }
}

// znajduje wierzcholki ktore sa na granicy partycji
void identify_boundary_vertices(FM_Context *context, bool *is_boundary)
{
    for (int i = 0; i < context->graph->vertices; i++)
    {
        // na poczatku zakladamy ze nie jest graniczny
        is_boundary[i] = false;

        // sprawdzamy wszystkich sasiadow
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];
            // jesli sasiad jest w innej partycji to wierzcholek jest graniczny
            if (context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                is_boundary[i] = true;
                break;
            }
        }
    }
}

// liczy poczatkowa liczbe przecietych krawedzi
int calculate_initial_cut(FM_Context *context)
{
    int cut_edges = 0;

    // przechodzimy przez wszystkie krawedzie
    for (int i = 0; i < context->graph->vertices; i++)
    {
        for (int j = 0; j < context->graph->nodes[i].neighbor_count; j++)
        {
            int neighbor = context->graph->nodes[i].neighbors[j];

            // liczymy tylko w jedna strone zeby uniknac podwojnego liczenia
            if (i < neighbor &&
                context->graph->nodes[i].part_id != context->graph->nodes[neighbor].part_id)
            {
                cut_edges++;
            }
        }
    }

    return cut_edges;
}

// liczy zysk z przeniesienia wierzcholka
int calculate_gain(FM_Context *context, int vertex, int target_part)
{
    // sprawdzamy parametry
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0;
    }

    int current_part = context->graph->nodes[vertex].part_id;
    int gain = 0;

    // sprawdzamy wszystkich sasiadow
    for (int i = 0; i < context->graph->nodes[vertex].neighbor_count; i++)
    {
        int neighbor = context->graph->nodes[vertex].neighbors[i];
        if (neighbor < 0 || neighbor >= context->graph->vertices)
        {
            continue;
        }

        int neighbor_part = context->graph->nodes[neighbor].part_id;

        // sasiad w partycji docelowej - zyskujemy bo krawedz nie bedzie przecieta
        if (neighbor_part == target_part)
        {
            gain++;
        }
        // sasiad w obecnej partycji - tracimy bo tworzymy nowe przeciecie
        else if (neighbor_part == current_part)
        {
            gain--;
        }
        // sasiedzi w innych partiach nie zmieniaja wyniku
    }

    return gain;
}

// sprawdza czy mozna przeniesc wierzcholek
int is_valid_move(FM_Context *context, int vertex, int target_part)
{
    // sprawdzamy poprawnosc parametrow
    if (vertex < 0 || vertex >= context->graph->vertices ||
        target_part < 0 || target_part >= context->graph->parts)
    {
        return 0;
    }

    int current_part = context->graph->nodes[vertex].part_id;

    // nie przenosimy zabanowanych wierzcholkow
    if (context->unmovable[vertex])
    {
        return 0;
    }

    // nie przenosimy zablokowanych wierzcholkow
    if (context->locked[vertex])
    {
        return 0;
    }

    // nie przenosimy do tej samej partycji
    if (current_part == target_part)
    {
        return 0;
    }

    // sprawdzamy ograniczenia rozmiaru
    int new_size_source = context->part_sizes[current_part] - 1;
    int new_size_target = context->part_sizes[target_part] + 1;
    int min_size = context->graph->min_count;
    int max_size = context->graph->max_count;

    // jesli narusza ograniczenia to nie mozemy wykonac ruchu
    if (new_size_source < min_size || new_size_target > max_size)
    {
        return 0;
    }

    return 1;
}

// wyswietla statystyki po optymalizacji
void print_cut_statistics(FM_Context *context)
{
    printf("\n\n  Initial cut: %d\n", context->initial_cut);
    printf("  Current cut: %d\n", context->current_cut);
    printf("  Improvement: %d\n", context->initial_cut - context->best_cut);
    printf("  Moves made: %d\n", context->moves_made);
}
