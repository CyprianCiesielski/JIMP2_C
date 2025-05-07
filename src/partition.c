#include "partition.h"

// funkcja pomocnicza do porownywania liczb calkowitych, uzywana przez qsort
static int compare_ints(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

// sprawdza czy wierzcholek nalezy do danej partycji
static int is_in_partition(const Partition_data *partition_data, int part_id, int vertex)
{
    if (!partition_data || part_id < 0 || part_id >= partition_data->parts_count)
    {
        return 0;
    }

    // przegladam wszystkie wierzcholki w partycji
    for (int i = 0; i < partition_data->parts[part_id].part_vertex_count; i++)
    {
        if (partition_data->parts[part_id].part_vertexes[i] == vertex)
        {
            return 1;
        }
    }
    return 0;
}

// tworzy liste sasiadow dla kazdego wierzcholka w danej partycji
int **get_part_neighbors(const Graph *graph, const Partition_data *partition_data, int part_id, int *size)
{
    if (!graph || !partition_data || !size || part_id < 0)
    {
        return NULL;
    }

    // pobieram liczbe wierzcholkow w partycji
    *size = partition_data->parts[part_id].part_vertex_count;
    if (*size <= 0)
    {
        return NULL;
    }

    // tworze tablice na posortowane wierzcholki
    int *vertices = malloc(*size * sizeof(int));
    if (!vertices)
    {
        return NULL;
    }

    // kopiuje i sortuje wierzcholki
    memcpy(vertices, partition_data->parts[part_id].part_vertexes, *size * sizeof(int));
    qsort(vertices, *size, sizeof(int), compare_ints);

    // alokuje tablice tablic sasiadow
    int **neighbors = malloc(*size * sizeof(int *));
    if (!neighbors)
    {
        free(vertices);
        return NULL;
    }

    // dla kazdego wierzcholka w kolejnosci posortowanej
    for (int i = 0; i < *size; i++)
    {
        int vertex = vertices[i];
        int max_neighbors = graph->nodes[vertex].neighbor_count;

        // tworze tymczasowa tablice na sasiadow
        int *temp_neighbors = malloc(max_neighbors * sizeof(int));
        int neighbor_count = 0;

        if (!temp_neighbors)
        {
            // sprzatam w przypadku bledu
            for (int j = 0; j < i; j++)
            {
                free(neighbors[j]);
            }
            free(neighbors);
            free(vertices);
            return NULL;
        }

        // szukam rzeczywistych sasiadow w tej samej partycji
        for (int j = 0; j < max_neighbors; j++)
        {
            int neighbor = graph->nodes[vertex].neighbors[j];
            if (is_in_partition(partition_data, part_id, neighbor))
            {
                temp_neighbors[neighbor_count++] = neighbor;
            }
        }

        // sortuje sasiadow jesli jakichs znalazlem
        if (neighbor_count > 0)
        {
            qsort(temp_neighbors, neighbor_count, sizeof(int), compare_ints);
        }

        // alokuje koncowa tablice dla wierzcholka i jego sasiadow (+ miejsce na sam wierzcholek i terminator)
        neighbors[i] = malloc((neighbor_count + 2) * sizeof(int));
        if (!neighbors[i])
        {
            // sprzatam w przypadku bledu
            for (int j = 0; j < i; j++)
            {
                free(neighbors[j]);
            }
            free(neighbors);
            free(vertices);
            free(temp_neighbors);
            return NULL;
        }

        // zapisuje sam wierzcholek jako pierwszy element
        neighbors[i][0] = vertex;

        // kopiuje posortowanych sasiadow
        for (int j = 0; j < neighbor_count; j++)
        {
            neighbors[i][j + 1] = temp_neighbors[j];
        }

        // dodaje terminator (-1) na koniec listy sasiadow
        neighbors[i][neighbor_count + 1] = -1;

        free(temp_neighbors);
    }

    free(vertices);
    return neighbors;
}

// inicjalizuje strukture danych partycji
void initialize_partition_data(Partition_data *partition_data, int parts)
{
    if (!partition_data)
    {
        fprintf(stderr, "Null partition_data pointer\n");
        return;
    }

    // ustawiam liczbe partycji
    partition_data->parts_count = parts;

    // alokuje pamiec na tablice partycji
    partition_data->parts = malloc(parts * sizeof(Part));
    if (!partition_data->parts)
    {
        perror("Memory allocation error for parts");
        exit(EXIT_FAILURE);
    }

    // inicjalizuje kazda partycje
    for (int i = 0; i < parts; i++)
    {
        partition_data->parts[i].part_id = i;
        partition_data->parts[i].part_vertexes = NULL;
        partition_data->parts[i].part_vertex_count = 0;
        partition_data->parts[i].capacity = 0;
    }
}

// zwalnia pamiec zaalokowana na dane partycji
void free_partition_data(Partition_data *partition_data, int parts)
{
    if (!partition_data)
    {
        return;
    }

    // zwalniam pamiec dla kazdej partycji
    if (partition_data->parts)
    {
        for (int i = 0; i < parts; i++)
        {
            free(partition_data->parts[i].part_vertexes);
        }
        free(partition_data->parts);
    }
}

// wypisuje informacje o partycjach
void print_partition_data(const Partition_data *partition_data)
{
    if (!partition_data)
        return;

    // wypisuje ogolne info o liczbie partycji
    printf("Number of parts: %d\n", partition_data->parts_count);

    // wypisuje wierzcholki w kazdej partycji
    for (int i = 0; i < partition_data->parts_count; i++)
    {
        printf("Part %d: ", i);
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++)
        {
            printf("%d ", partition_data->parts[i].part_vertexes[j]);
        }
        printf("\n");
    }

    // wypisuje liczbe wierzcholkow w kazdej partycji
    printf("\nVertex count in each part:\n");
    for (int i = 0; i < partition_data->parts_count; i++)
    {
        printf("Part %d: %d\n", i, partition_data->parts[i].part_vertex_count);
    }
}

// dodaje wierzcholek do partycji z automatycznym zwiekszaniem tablicy jesli potrzeba
void add_partition_data(Partition_data *partition_data, int part_id, int vertex)
{
    if (!partition_data || part_id < 0 || part_id >= partition_data->parts_count)
    {
        fprintf(stderr, "Invalid partition data or part_id\n");
        return;
    }

    // sprawdzam czy trzeba zainicjalizowac tablice wierzcholkow
    if (partition_data->parts[part_id].part_vertexes == NULL)
    {
        // inicjalna alokacja z pojemnoscia 128 elementow
        partition_data->parts[part_id].capacity = 128;
        partition_data->parts[part_id].part_vertexes = malloc(partition_data->parts[part_id].capacity * sizeof(int));
        if (!partition_data->parts[part_id].part_vertexes)
        {
            perror("Memory allocation error for part vertices");
            exit(EXIT_FAILURE);
        }
        partition_data->parts[part_id].part_vertex_count = 0;
    }
    // sprawdzam czy trzeba powiekszyc tablice
    else if (partition_data->parts[part_id].part_vertex_count >= partition_data->parts[part_id].capacity)
    {
        // podwajam pojemnosc
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

    // dodaje nowy wierzcholek
    partition_data->parts[part_id].part_vertexes[partition_data->parts[part_id].part_vertex_count++] = vertex;
}

