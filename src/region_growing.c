#include "region_growing.h"

// sprawdza czy kolejka jest pusta
int is_empty(struct Queue *queue)
{
    return queue->front == queue->rear;
}

// dodaje element do kolejki
void add_to_queue(struct Queue *queue, int item)
{
    if (queue->rear < queue->max_size)
    {
        queue->items[queue->rear++] = item;
    }
    else
    {
        fprintf(stderr, "Kolejka jest pelna\n");
    }
}

// usuwa element z poczatku kolejki
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

// generuje losowe punkty startowe dla partycji
int *generate_seed_points(Graph *graph, int parts)
{
    srand(time(NULL)); // inicjalizuje generator losowy
    int *seed_points = malloc(parts * sizeof(int));
    if (seed_points == NULL)
    {
        perror("Blad alokacji pamieci dla punktow startowych");
        exit(EXIT_FAILURE);
    }

    // losuje wierzcholki jako punkty startowe
    for (int i = 0; i < parts; i++)
    {
        seed_points[i] = rand() % graph->vertices;
    }

    // oznacza punkty startowe jako nalezace do odpowiednich partycji
    for (int i = 0; i < parts; i++)
    {
        graph->nodes[seed_points[i]].part_id = i;
    }

    return seed_points;
}

// glowny algorytm podzialu grafu metoda rozrostu regionow
int region_growing(Graph *graph, int parts, Partition_data *partition_data, float accuracy)
{
    // sprawdzam czy liczba partycji jest prawidlowa
    if (parts > graph->vertices)
    {
        perror("Liczba czesci nie moze byc wieksza od liczby wierzcholkow");
        exit(EXIT_FAILURE);
    }

    // inicjalizuje zmienne
    int *seed_points = generate_seed_points(graph, parts);
    int *visited = malloc(graph->vertices * sizeof(int));

    if (visited == NULL)
    {
        perror("Blad alokacji pamieci dla odwiedzonych wezlow");
        free(seed_points);
        exit(EXIT_FAILURE);
    }

    // zeruje tablice odwiedzonych wierzcholkow
    memset(visited, 0, graph->vertices * sizeof(int));

    // tworze tablice frontow dla kazdej partycji
    int **frontier = malloc(parts * sizeof(int *));
    int *frontier_size = malloc(parts * sizeof(int));
    int *frontier_capacity = malloc(parts * sizeof(int));

    if (!frontier || !frontier_size || !frontier_capacity)
    {
        perror("Blad alokacji pamieci dla frontow partycji");
        free(seed_points);
        free(visited);
        exit(EXIT_FAILURE);
    }

    // inicjalizuje tablice frontow
    for (int i = 0; i < parts; i++)
    {
        frontier_size[i] = 0;
        frontier_capacity[i] = 10; // poczatkowy rozmiar
        frontier[i] = malloc(frontier_capacity[i] * sizeof(int));
        if (!frontier[i])
        {
            perror("Blad alokacji pamieci dla frontu partycji");
            // sprzatam juz zaalokowane fronty
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

    // resetuje przypisanie partycji dla wszystkich wierzcholkow
    for (int i = 0; i < graph->vertices; i++)
    {
        graph->nodes[i].part_id = -1; // -1 to brak przypisania
    }

    // inicjalizuje liczniki wierzcholkow w partycjach
    int *part_counts = calloc(parts, sizeof(int));
    if (!part_counts)
    {
        perror("Blad alokacji pamieci dla licznikow partycji");
        // sprzatam
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

    // dodaje punkty startowe do frontow partycji
    printf("Seed points: ");
    for (int i = 0; i < parts; i++)
    {
        printf("%d ", seed_points[i]);
        visited[seed_points[i]] = 1;
        graph->nodes[seed_points[i]].part_id = i;
        add_partition_data(partition_data, i, seed_points[i]);
        part_counts[i]++;

        // dodaje sasiadow punktu startowego do frontu partycji
        Node *node = &graph->nodes[seed_points[i]];
        for (int j = 0; j < node->neighbor_count; j++)
        {
            int neighbor = node->neighbors[j];
            if (!visited[neighbor])
            {
                // powiekszam front jesli potrzeba
                if (frontier_size[i] >= frontier_capacity[i])
                {
                    frontier_capacity[i] *= 2;
                    frontier[i] = realloc(frontier[i], frontier_capacity[i] * sizeof(int));
                    if (!frontier[i])
                    {
                        perror("Blad realokacji frontu partycji");
                        exit(EXIT_FAILURE);
                    }
                }
                frontier[i][frontier_size[i]++] = neighbor;
            }
        }
    }
    printf("\n");

    // wyswietlam liczbe sasiadow punktow startowych
    printf("Neighbor counts: ");
    for (int i = 0; i < parts; i++)
    {
        printf("seed %d: %d neighbors, ", seed_points[i], graph->nodes[seed_points[i]].neighbor_count);
    }
    printf("\n");

    // obliczam srednia liczbe wierzcholkow na partycje
    float avg_vertices_per_part = (float)graph->vertices / parts;

    // obliczam min i max rozmiary partycji na podstawie parametru dokladnosci
    int min_vertices_per_part = (int)((avg_vertices_per_part * (1.0 - accuracy)));
    if ((float)min_vertices_per_part < (avg_vertices_per_part * (1.0 - accuracy)))
    {
        min_vertices_per_part += 1;
    }
    int max_vertices_per_part = (int)(avg_vertices_per_part * (1.0 + accuracy));

    printf("Average vertices per part: %.2f (min: %d, max: %d)\n",
           avg_vertices_per_part, min_vertices_per_part, max_vertices_per_part);

    // glowna petla algorytmu rozrostu regionow
    int iterations = 0;
    int unassigned = graph->vertices - parts; // wszystkie wierzcholki minus punkty startowe

    while (unassigned > 0 && iterations < graph->vertices * 2)
    {
        // szukam partycji z najmniejsza liczba wierzcholkow, ktora ma jeszcze dostepny front
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

        // jesli nie znaleziono odpowiedniej partycji, sprawdzam czy sa jakies fronty
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

            // konczymy jesli nie ma zadnych frontow
            if (!any_frontier)
            {
                printf("No available frontiers, terminating\n");
                break;
            }

            // jesli wszystkie partycje osiagnely gorny limit, zwiekszamy go
            if (part_counts[min_part] >= max_vertices_per_part)
            {
                max_vertices_per_part++;
                printf("Increasing max_vertices_per_part to %d\n", max_vertices_per_part);
            }
        }

        // rozrastam wybrana partycje o jeden wierzcholek
        int current = frontier[min_part][--frontier_size[min_part]];

        if (!visited[current])
        {
            visited[current] = 1;
            graph->nodes[current].part_id = min_part;
            add_partition_data(partition_data, min_part, current);
            part_counts[min_part]++;
            unassigned--;

            // dodaje sasiadow do frontu
            Node *node = &graph->nodes[current];
            for (int i = 0; i < node->neighbor_count; i++)
            {
                int neighbor = node->neighbors[i];

                // sprawdzam czy sasiad ma poprawny indeks
                if (neighbor < 0 || neighbor >= graph->vertices)
                {
                    printf("ERROR: Invalid neighbor index %d for node %d\n", neighbor, current);
                    continue;
                }

                if (!visited[neighbor])
                {
                    // dodaje do frontu partycji, powiekszajac go jesli potrzeba
                    if (frontier_size[min_part] >= frontier_capacity[min_part])
                    {
                        frontier_capacity[min_part] *= 2;
                        frontier[min_part] = realloc(frontier[min_part],
                                                     frontier_capacity[min_part] * sizeof(int));
                        if (!frontier[min_part])
                        {
                            perror("Blad realokacji frontu partycji");
                            exit(EXIT_FAILURE);
                        }
                    }
                    frontier[min_part][frontier_size[min_part]++] = neighbor;
                }
            }
        }

        iterations++;

        // co 100 iteracji sprawdzam czy osiagnelismy dobra rownowage
        if (iterations % 100 == 0)
        {
            // znajduje najwieksza i najmniejsza liczbe wierzcholkow
            int min_count = graph->vertices;
            int max_count = 0;

            for (int i = 0; i < parts; i++)
            {
                if (part_counts[i] < min_count)
                    min_count = part_counts[i];
                if (part_counts[i] > max_count)
                    max_count = part_counts[i];
            }

            // sprawdzam czy osiagnelismy dobra rownowage
            if (min_count >= min_vertices_per_part && unassigned == 0)
            {
                printf("Good balance achieved, terminating\n");
                break;
            }
        }
    }

    // sprawdzam czy sa nieprzypisane wierzcholki
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
        // przypisuje pozostale wierzcholki do partycji
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == -1)
            {
                // najpierw sprawdzam czy wierzcholek ma sasiadow w jakiejs partycji
                Node *node = &graph->nodes[i];
                int has_neighbor_in_partition = 0;
                int neighbor_part = -1;
                int smallest_neighbor_part = -1;
                int smallest_neighbor_count = graph->vertices;

                // szukam partycji sasiadow
                for (int j = 0; j < node->neighbor_count; j++)
                {
                    int neighbor = node->neighbors[j];
                    if (neighbor >= 0 && neighbor < graph->vertices && graph->nodes[neighbor].part_id != -1)
                    {
                        has_neighbor_in_partition = 1;
                        neighbor_part = graph->nodes[neighbor].part_id;

                        // szukam najmniejszej partycji wsrod sasiadow
                        if (part_counts[neighbor_part] < smallest_neighbor_count)
                        {
                            smallest_neighbor_count = part_counts[neighbor_part];
                            smallest_neighbor_part = neighbor_part;
                        }
                    }
                }

                // jesli ma sasiada w jakiejs partycji i ta partycja nie przekracza limitu,
                // przypisuje do tej partycji
                if (has_neighbor_in_partition && part_counts[smallest_neighbor_part] < max_vertices_per_part)
                {
                    graph->nodes[i].part_id = smallest_neighbor_part;
                    add_partition_data(partition_data, smallest_neighbor_part, i);
                    part_counts[smallest_neighbor_part]++;
                }
                else
                {
                    // przypisuje do najmniejszej partycji
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

    // wypisuje ostrzezenie jesli osiagnieto limit iteracji
    if (iterations >= graph->vertices * 3)
    {
        printf("WARNING: Loop terminated due to iteration limit\n");
    }

    // wypisuje rozmiary partycji na koniec
    printf("Final partition sizes: ");
    for (int i = 0; i < parts; i++)
    {
        printf("part %d: %d nodes (%.2f%% of average), ",
               i, part_counts[i], (part_counts[i] * 100.0) / avg_vertices_per_part);
    }
    printf("\n");

    // sprawdzam czy spelnione sa wymagania dokladnosci
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

    // wyswietlam ostrzezenie jesli wymagania nie sa spelnione
    if (min_ratio < (1.0 - accuracy) || max_ratio > (1.0 + accuracy))
    {
        success = 0;
        printf("Warning: Final partition ratios (min: %.2f, max: %.2f) outside accuracy range (%.2f - %.2f)\n",
               min_ratio, max_ratio, 1.0 - accuracy, 1.0 + accuracy);
    }

    // sprzatam
    free(visited);
    free(seed_points);
    free(part_counts);
    for (int i = 0; i < parts; i++)
    {
        for (int i = 0; i < parts; i++)
        {
            free(frontier[i]);
        }
        free(frontier);
        free(frontier_size);
        free(frontier_capacity);
        return success; // zwracam czy udalo sie spelnic wymagania dokladnosci
    }
    return 0; // zwracam 0 jesli nie udalo sie spelnic wymagania dokladnosci
}
// sprawdza spojnosc partycji w grafie
void check_partition_connectivity(Graph *graph, int parts)
{
    printf("\n--- Checking partition connectivity ---\n");
    int all_connected = 1;

    // sprawdzam kazda partycje po kolei
    for (int p = 0; p < parts; p++)
    {
        // licze wierzcholki w partycji
        int count = 0;
        for (int i = 0; i < graph->vertices; i++)
            if (graph->nodes[i].part_id == p)
                count++;

        // pomijam puste partycje
        if (count == 0)
        {
            printf("Partition %d is empty - skipping\n", p);
            continue;
        }

        // przygotowuje struktury do BFS
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

        // szukam pierwszego wierzcholka w partycji
        int start = -1;
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == p)
            {
                start = i;
                break;
            }
        }

        // wykonuje przejscie BFS
        int front = 0, rear = 0;
        queue[rear++] = start;
        visited[start] = true;
        int visited_count = 1;

        while (front < rear)
        {
            int current = queue[front++];

            // przegladam sasiadow
            for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
            {
                int neighbor = graph->nodes[current].neighbors[i];
                // dodaje do kolejki tylko sasiadow z tej samej partycji
                if (graph->nodes[neighbor].part_id == p && !visited[neighbor])
                {
                    visited[neighbor] = true;
                    queue[rear++] = neighbor;
                    visited_count++;
                }
            }
        }

        // sprawdzam czy wszystkie wierzcholki zostaly odwiedzone
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