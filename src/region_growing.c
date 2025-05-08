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

// generuje punkty startowe dla partycji, staramy sie zeby byly od siebie oddalone
int *generate_seed_points(Graph *graph, int parts)
{
    srand(time(NULL));
    int *seed_points = malloc(parts * sizeof(int));
    if (seed_points == NULL)
    {
        perror("Blad alokacji pamieci dla punktow startowych");
        exit(EXIT_FAILURE);
    }

    // losujemy pierwszy punkt
    seed_points[0] = rand() % graph->vertices;

    // dla kazdego kolejnego punktu szukamy takiego, ktory jest malo polaczony z poprzednimi
    for (int i = 1; i < parts; i++)
    {
        int best_vertex = -1;
        int min_connections = graph->vertices; // maksymalna mozliwa liczba polaczen

        // sprawdzamy 100 losowych kandydatow
        for (int j = 0; j < 100; j++)
        {
            int candidate = rand() % graph->vertices;

            // liczymy polaczenia z juz wybranymi punktami
            int connections = 0;
            for (int k = 0; k < i; k++)
            {
                int seed = seed_points[k];

                // sprawdzamy czy jest bezposrednie polaczenie
                for (int l = 0; l < graph->nodes[candidate].neighbor_count; l++)
                {
                    if (graph->nodes[candidate].neighbors[l] == seed)
                    {
                        connections++;
                        break;
                    }
                }
            }

            // wybieramy wierzcholek z najmniejsza liczba polaczen
            if (connections < min_connections)
            {
                min_connections = connections;
                best_vertex = candidate;

                // jesli znalezlismy calkowicie niepodlaczony punkt, to super
                if (min_connections == 0)
                    break;
            }
        }

        seed_points[i] = best_vertex;
    }

    // oznaczamy punkty startowe jako nalezace do odpowiednich partycji
    for (int i = 0; i < parts; i++)
    {
        graph->nodes[seed_points[i]].part_id = i;
    }

    return seed_points;
}

// glowny algorytm podzialu grafu metoda rozrostu regionow
int region_growing(Graph *graph, int parts, Partition_data *partition_data, float accuracy)
{
    // sprawdzamy czy liczba partycji ma sens
    if (parts > graph->vertices)
    {
        perror("Liczba czesci nie moze byc wieksza od liczby wierzcholkow");
        exit(EXIT_FAILURE);
    }

    // inicjalizujemy zmienne
    int *seed_points = generate_seed_points(graph, parts);
    int *visited = malloc(graph->vertices * sizeof(int));

    if (visited == NULL)
    {
        perror("Blad alokacji pamieci dla odwiedzonych wezlow");
        free(seed_points);
        exit(EXIT_FAILURE);
    }

    // zerujemy tablice odwiedzonych
    memset(visited, 0, graph->vertices * sizeof(int));

    // tworzymy tablice frontow dla kazdej partycji
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

    // inicjalizujemy fronty z poczatkowym rozmiarem
    for (int i = 0; i < parts; i++)
    {
        frontier_size[i] = 0;
        frontier_capacity[i] = 10; // startowy rozmiar
        frontier[i] = malloc(frontier_capacity[i] * sizeof(int));
        if (!frontier[i])
        {
            perror("Blad alokacji pamieci dla frontu partycji");
            // sprzatamy wczesniej zaalokowane
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

    // resetujemy przypisania partycji
    for (int i = 0; i < graph->vertices; i++)
    {
        graph->nodes[i].part_id = -1; // -1 oznacza brak przypisania
    }

    // inicjalizujemy liczniki wierzcholkow w partiach
    int *part_counts = calloc(parts, sizeof(int));
    if (!part_counts)
    {
        perror("Blad alokacji pamieci dla licznikow partycji");
        // sprzatamy
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

    // dodajemy punkty startowe do frontow partycji
    // printf("Seed points: ");
    for (int i = 0; i < parts; i++)
    {
        // printf("%d ", seed_points[i]);
        visited[seed_points[i]] = 1;
        graph->nodes[seed_points[i]].part_id = i;
        add_partition_data(partition_data, i, seed_points[i]);
        part_counts[i]++;

        // dodajemy sasiadow punktu startowego do frontu
        Node *node = &graph->nodes[seed_points[i]];
        for (int j = 0; j < node->neighbor_count; j++)
        {
            int neighbor = node->neighbors[j];
            if (!visited[neighbor])
            {
                // zwiekszamy pojemnosc frontu jesli potrzeba
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
    // printf("\n");

    // wyswietlamy liczbe sasiadow punktow startowych
    // printf("Neighbor counts: ");
    for (int i = 0; i < parts; i++)
    {
        // printf("seed %d: %d neighbors, ", seed_points[i], graph->nodes[seed_points[i]].neighbor_count);
    }
    // printf("\n");

    // obliczamy srednia liczbe wierzcholkow na partycje
    float avg_vertices_per_part = (float)graph->vertices / parts;

    // obliczamy min i max bazujac na dokladnosci
    int min_vertices_per_part = (int)((avg_vertices_per_part * (1.0 - accuracy)));
    if ((float)min_vertices_per_part < (avg_vertices_per_part * (1.0 - accuracy)))
    {
        min_vertices_per_part += 1;
    }
    int max_vertices_per_part = (int)(avg_vertices_per_part * (1.0 + accuracy));

    // printf("Average vertices per part: %.2f (min: %d, max: %d)\n",avg_vertices_per_part, min_vertices_per_part, max_vertices_per_part);

    // glowna petla algorytmu
    int iterations = 0;
    int unassigned = graph->vertices - parts; // wszystkie wierzcholki minus punkty startowe

    // optymalizacja - trzymamy liste aktywnych partycji
    int *active_partitions = malloc(parts * sizeof(int));
    int active_count = parts; // na poczatku wszystkie sa aktywne

    for (int i = 0; i < parts; i++)
        active_partitions[i] = i;

    while (unassigned > 0 && iterations < graph->vertices * 2)
    {
        int min_part = -1;
        int min_size = graph->vertices;

        // sprawdzamy tylko aktywne partycje
        for (int idx = 0; idx < active_count; idx++)
        {
            int i = active_partitions[idx];
            if (frontier_size[i] > 0 && part_counts[i] < max_vertices_per_part)
            {
                if (min_part == -1 || part_counts[i] < min_size)
                {
                    min_part = i;
                    min_size = part_counts[i];
                }
            }
        }

        // jesli partycja staje sie nieaktywna, usuwamy ja z listy
        if (frontier_size[min_part] == 0)
        {
            for (int idx = 0; idx < active_count; idx++)
            {
                if (active_partitions[idx] == min_part)
                {
                    active_partitions[idx] = active_partitions[active_count - 1];
                    active_count--;
                    break;
                }
            }
        }

        // wybieramy tylko wierzcholek, ktory ma sasiada w partycji
        int valid_vertex_found = 0;
        int current = -1;

        // sprawdzamy po kolei wierzcholki z frontu
        for (int i = 0; i < frontier_size[min_part]; i++)
        {
            int candidate = frontier[min_part][i];

            // sprawdzamy czy kandydat ma sasiada w partycji
            if (!visited[candidate])
            {
                int has_neighbor_in_partition = 0;
                Node *node = &graph->nodes[candidate];

                for (int j = 0; j < node->neighbor_count; j++)
                {
                    int neighbor = node->neighbors[j];
                    if (graph->nodes[neighbor].part_id == min_part)
                    {
                        has_neighbor_in_partition = 1;
                        break;
                    }
                }

                if (has_neighbor_in_partition)
                {
                    current = candidate;
                    valid_vertex_found = 1;

                    // usuwamy go z frontu przez zamiane z ostatnim - szybciej niz przesuwanie
                    frontier[min_part][i] = frontier[min_part][frontier_size[min_part] - 1];
                    frontier_size[min_part]--;
                    break;
                }
            }
        }

        // jesli nie znalezlismy odpowiedniego wierzcholka, pomijamy te partycje
        if (!valid_vertex_found)
        {
            // reset frontu
            frontier_size[min_part] = 0;
            continue;
        }

        if (!visited[current])
        {
            visited[current] = 1;
            graph->nodes[current].part_id = min_part;
            add_partition_data(partition_data, min_part, current);
            part_counts[min_part]++;
            unassigned--;

            // dodajemy sasiadow do frontu
            Node *node = &graph->nodes[current];
            for (int i = 0; i < node->neighbor_count; i++)
            {
                int neighbor = node->neighbors[i];

                // sprawdzamy indeks sasiada
                if (neighbor < 0 || neighbor >= graph->vertices)
                {
                    // printf("ERROR: Invalid neighbor index %d for node %d\n", neighbor, current);
                    continue;
                }

                if (!visited[neighbor])
                {
                    // dodajemy do frontu, powiekszajac w razie potrzeby
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

        // co jakis czas sprawdzamy postepy
        if (iterations % 100 == 0)
        {
            // szukamy min i max liczby wierzcholkow
            int min_count = graph->vertices;
            int max_count = 0;

            for (int i = 0; i < parts; i++)
            {
                if (part_counts[i] < min_count)
                    min_count = part_counts[i];
                if (part_counts[i] > max_count)
                    max_count = part_counts[i];
            }

            // sprawdzamy czy jestesmy juz zbalansowani
            if (min_count >= min_vertices_per_part && unassigned == 0)
            {
                // printf("Good balance achieved, terminating\n");
                break;
            }
        }
    }

    free(active_partitions);

    // sprawdzamy czy zostaly jakies nieprzypisane wierzcholki
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
        // printf("Assigning %d unassigned vertices...\n", unassigned_count);

        // tworzymy liste nieprzypisanych wierzcholkow
        int *unassigned_vertices = malloc(unassigned_count * sizeof(int));
        int idx = 0;

        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == -1)
            {
                unassigned_vertices[idx++] = i;
            }
        }

        // probujemy przyporzadkowac z zachowaniem spojnosci
        int assigned = 1; // flaga czy cos przypisalismy w tej iteracji

        while (assigned && idx > 0)
        {
            assigned = 0;

            // sprawdzamy kazdy nieprzypisany wierzcholek
            for (int i = 0; i < idx; i++)
            {
                int v = unassigned_vertices[i];

                if (graph->nodes[v].part_id != -1)
                {
                    // juz przypisany w tej iteracji
                    continue;
                }

                Node *node = &graph->nodes[v];
                int smallest_neighbor_part = -1;
                int smallest_neighbor_count = graph->vertices;

                // szukamy sasiedniej partycji z najmniejsza liczba wierzcholkow
                for (int j = 0; j < node->neighbor_count; j++)
                {
                    int neighbor = node->neighbors[j];

                    if (neighbor >= 0 && neighbor < graph->vertices &&
                        graph->nodes[neighbor].part_id != -1)
                    {
                        int neighbor_part = graph->nodes[neighbor].part_id;

                        if (part_counts[neighbor_part] < smallest_neighbor_count)
                        {
                            smallest_neighbor_count = part_counts[neighbor_part];
                            smallest_neighbor_part = neighbor_part;
                        }
                    }
                }

                // jesli znalezlismy sasiednia partycje, przypisujemy
                if (smallest_neighbor_part != -1)
                {
                    graph->nodes[v].part_id = smallest_neighbor_part;
                    add_partition_data(partition_data, smallest_neighbor_part, v);
                    part_counts[smallest_neighbor_part]++;
                    assigned = 1;

                    // oznaczamy jako przypisany przez -2
                    unassigned_vertices[i] = -2;
                }
            }

            // usuwamy przypisane wierzcholki z listy
            int new_idx = 0;
            for (int i = 0; i < idx; i++)
            {
                if (unassigned_vertices[i] != -2)
                {
                    unassigned_vertices[new_idx++] = unassigned_vertices[i];
                }
            }
            idx = new_idx;
        }

        // jesli nadal zostaly nieprzypisane wierzcholki, przypisujemy na sile
        if (idx > 0)
        {
            // printf("Warning: %d vertices couldn't be assigned while maintaining connectivity\n", idx);

            for (int i = 0; i < idx; i++)
            {
                int v = unassigned_vertices[i];

                // dajemy do najmniejszej partycji
                int min_part = 0;
                for (int j = 1; j < parts; j++)
                {
                    if (part_counts[j] < part_counts[min_part])
                    {
                        min_part = j;
                    }
                }

                graph->nodes[v].part_id = min_part;
                add_partition_data(partition_data, min_part, v);
                part_counts[min_part]++;
            }
        }

        free(unassigned_vertices);
    }

    // ostrzegamy jesli przekroczono limit iteracji
    if (iterations >= graph->vertices * 3)
    {
        printf("WARNING: Loop terminated due to iteration limit\n");
    }

    // wyswietlamy koncowe rozmiary partycji
    // printf("Final partition sizes: ");
    // for (int i = 0; i < parts; i++)
    // {
    //     printf("part %d: %d nodes (%.2f%% of average), ", i, part_counts[i], (part_counts[i] * 100.0) / avg_vertices_per_part);
    // }
    // printf("\n");

    // sprawdzamy czy spelnilismy wymagania dokladnosci
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

    // ostrzezenie jesli nie spelnilismy wymagan
    if (min_ratio < (1.0 - accuracy) || max_ratio > (1.0 + accuracy))
    {
        success = 0;
        // printf("Warning: Final partition ratios (min: %.2f, max: %.2f) outside accuracy range (%.2f - %.2f)\n",min_ratio, max_ratio, 1.0 - accuracy, 1.0 + accuracy);
    }

    // weryfikujemy spojnosc i naprawiamy jesli trzeba
    // printf("\nVerifying partition connectivity...\n");
    int all_connected = 1;

    // sprawdzamy spojnosc dla kazdej partycji
    for (int p = 0; p < parts; p++)
    {
        int is_connected = verify_partition_connectivity(graph, p);
        if (!is_connected)
        {
            all_connected = 0;
            // printf("Partition %d is not connected - fixing...\n", p);
            fix_disconnected_partition(graph, p, part_counts);
        }
    }

    if (!all_connected)
    {
        printf("Fixed disconnected partitions. Re-verifying connectivity...\n");
        check_partition_connectivity(graph, parts);
    }

    // sprzatanie
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

// sprawdza spojnosc pojedynczej partycji
int verify_partition_connectivity(Graph *graph, int part_id)
{
    // liczymy wierzcholki w partycji tylko raz
    int *partition_node_counts = NULL;
    if (!partition_node_counts)
    {
        partition_node_counts = calloc(graph->parts, sizeof(int));
        // zliczamy wierzcholki w kazdej partycji
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id >= 0 && graph->nodes[i].part_id < graph->parts)
                partition_node_counts[graph->nodes[i].part_id]++;
        }
    }

    // ilosc wierzcholkow w sprawdzanej partycji
    int count = partition_node_counts[part_id];

    // pusta lub z jednym wierzcholkiem jest spojna
    if (count <= 1)
    {
        free(partition_node_counts);
        return 1;
    }

    // przygotowujemy struktury do BFS
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *queue = malloc(graph->vertices * sizeof(int));

    if (!visited || !queue)
    {
        if (visited)
            free(visited);
        if (queue)
            free(queue);
        free(partition_node_counts);
        return 0;
    }

    // szukamy pierwszego wierzcholka w partycji
    int start = -1;
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id)
        {
            start = i;
            break;
        }
    }

    // robimy BFS z tego wierzcholka
    int front = 0, rear = 0;
    queue[rear++] = start;
    visited[start] = true;
    int visited_count = 1;

    while (front < rear)
    {
        int current = queue[front++];

        // sprawdzamy sasiadow
        for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
        {
            int neighbor = graph->nodes[current].neighbors[i];
            // do kolejki dodajemy tylko sasiadow z tej samej partycji
            if (graph->nodes[neighbor].part_id == part_id && !visited[neighbor])
            {
                visited[neighbor] = true;
                queue[rear++] = neighbor;
                visited_count++;
            }
        }
    }

    free(visited);
    free(queue);
    free(partition_node_counts);

    // partycja jest spojna jesli odwiedzilismy wszystkie wierzcholki
    return (visited_count == count);
}

// naprawia niespojne partycje
void fix_disconnected_partition(Graph *graph, int part_id, int *part_counts)
{
    // znajdujemy wszystkie komponenty w partycji
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *component_id = malloc(graph->vertices * sizeof(int));
    int *component_size = calloc(graph->vertices, sizeof(int));
    int component_count = 0;

    if (!visited || !component_id || !component_size)
    {
        if (visited)
            free(visited);
        if (component_id)
            free(component_id);
        if (component_size)
            free(component_size);
        return;
    }

    // przechodzimy przez wierzcholki partycji
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id && !visited[i])
        {
            // znalezlismy nowy komponent
            int *queue = malloc(graph->vertices * sizeof(int));
            if (!queue)
            {
                free(visited);
                free(component_id);
                free(component_size);
                return;
            }

            // BFS dla tego komponentu
            int front = 0, rear = 0;
            queue[rear++] = i;
            visited[i] = true;
            component_id[i] = component_count;
            component_size[component_count]++;

            while (front < rear)
            {
                int current = queue[front++];

                for (int j = 0; j < graph->nodes[current].neighbor_count; j++)
                {
                    int neighbor = graph->nodes[current].neighbors[j];
                    if (graph->nodes[neighbor].part_id == part_id && !visited[neighbor])
                    {
                        visited[neighbor] = true;
                        queue[rear++] = neighbor;
                        component_id[neighbor] = component_count;
                        component_size[component_count]++;
                    }
                }
            }

            free(queue);
            component_count++;
        }
    }

    // zostawiamy najwiekszy komponent, reszta idzie do innych partycji
    int largest_component = 0;
    for (int i = 1; i < component_count; i++)
    {
        if (component_size[i] > component_size[largest_component])
        {
            largest_component = i;
        }
    }

    // przepisujemy pozostale komponenty do najblizszych partycji
    for (int i = 0; i < graph->vertices; i++)
    {
        if (graph->nodes[i].part_id == part_id && component_id[i] != largest_component)
        {
            // szukamy sasiedniej partycji
            int best_part = -1;
            Node *node = &graph->nodes[i];

            for (int j = 0; j < node->neighbor_count; j++)
            {
                int neighbor = node->neighbors[j];
                int neighbor_part = graph->nodes[neighbor].part_id;

                if (neighbor_part != part_id && neighbor_part != -1)
                {
                    if (best_part == -1 || part_counts[neighbor_part] < part_counts[best_part])
                    {
                        best_part = neighbor_part;
                    }
                }
            }

            // jesli znalezlismy sasiednia partycje, przypisujemy
            if (best_part != -1)
            {
                part_counts[part_id]--;
                part_counts[best_part]++;
                graph->nodes[i].part_id = best_part;
            }
        }
    }

    free(visited);
    free(component_id);
    free(component_size);
}

// sprawdza spojnosc wszystkich partycji w grafie
void check_partition_connectivity(Graph *graph, int parts)
{
    // printf("\n--- Checking partition connectivity ---\n");
    int all_connected = 1;

    // alokujemy struktury tylko raz dla wszystkich partycji
    bool *visited = calloc(graph->vertices, sizeof(bool));
    int *queue = malloc(graph->vertices * sizeof(int));

    if (!visited || !queue)
    {
        // printf("Failed to allocate memory for connectivity check\n");
        if (visited)
            free(visited);
        if (queue)
            free(queue);
        return;
    }

    // sprawdzamy kazda partycje po kolei
    for (int p = 0; p < parts; p++)
    {
        // zerujemy tablice odwiedzonych dla nowej partycji
        memset(visited, 0, graph->vertices * sizeof(bool));

        // liczymy ile wierzcholkow jest w tej partycji
        int vertices_in_part = 0;
        int vertices_visited = 0;

        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == p)
                vertices_in_part++;
        }

        // jesli partycja ma 0 lub 1 wierzcholek, jest spojna
        if (vertices_in_part <= 1)
        {
            // printf("Partition %d: CONNECTED (visited %d/%d vertices)\n",p, vertices_in_part, vertices_in_part);
            continue;
        }

        // szukamy pierwszego wierzcholka w partycji
        int start_vertex = -1;
        for (int i = 0; i < graph->vertices; i++)
        {
            if (graph->nodes[i].part_id == p)
            {
                start_vertex = i;
                break;
            }
        }

        // BFS rozpoczynajac od znalezionego wierzcholka
        int front = 0, rear = 0;
        queue[rear++] = start_vertex;
        visited[start_vertex] = true;
        vertices_visited = 1;

        while (front < rear)
        {
            int current = queue[front++];

            for (int i = 0; i < graph->nodes[current].neighbor_count; i++)
            {
                int neighbor = graph->nodes[current].neighbors[i];

                // dodajemy do kolejki tylko sasiadow z tej samej partycji
                if (graph->nodes[neighbor].part_id == p && !visited[neighbor])
                {
                    visited[neighbor] = true;
                    queue[rear++] = neighbor;
                    vertices_visited++;
                }
            }
        }

        // sprawdzamy czy wszystkie wierzcholki partycji zostaly odwiedzone
        int is_connected = (vertices_visited == vertices_in_part);

        // printf("Partition %d: %s (visited %d/%d vertices)\n", p, is_connected ? "CONNECTED" : "DISCONNECTED", vertices_visited, vertices_in_part);

        if (!is_connected)
            all_connected = 0;
    }

    // podsumowanie
    // printf("Overall partition connectivity: %s\n", all_connected ? "VALID" : "INVALID");
    // printf("--- End of connectivity check ---\n");

    // zwalniamy pamiec
    free(visited);
    free(queue);
}