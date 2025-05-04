#include "file_reader.h"
#include <sys/types.h>
#include <unistd.h>

int MAX_BUFFOR = 40000;
// Funkcja pomocnicza do dekodowania liczby w formacie vByte
int decode_vbyte(FILE *file)
{
    int value = 0;
    int shift = 0;
    uint8_t byte;

    while (fread(&byte, sizeof(uint8_t), 1, file) == 1)
    {
        value |= (byte & 0x7F) << shift; // Dodaj 7 bitów do wartości
        if ((byte & 0x80) == 0)
        { // Jeśli MSB = 0, to koniec liczby
            break;
        }
        shift += 7; // Przesuń o 7 bitów
    }

    return value;
}

// Funkcja do odczytu pliku binarnego wiersz po wierszu
void read_binary(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Nie można otworzyć pliku binarnego do odczytu");
        exit(EXIT_FAILURE);
    }

    const uint64_t separator = 0xDEADBEEFCAFEBABE;
    uint64_t read_separator;

    // Odczyt pierwszej linii (liczba wierzchołków)
    int num_vertices = decode_vbyte(file);
    printf("%d\n\n", num_vertices);  // Added extra \n

    // Sprawdź separator po pierwszej linii
    fread(&read_separator, sizeof(uint64_t), 1, file);
    if (read_separator != separator) {
        fprintf(stderr, "Błąd: brak separatora po pierwszej linii\n");
        fclose(file);
        return;
    }

    // Odczyt drugiej linii
    int val;
    while (1) {
        val = decode_vbyte(file);
        printf("%d", val);
        
        // Sprawdź czy następny bajt to początek separatora
        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1) {
            if (peek == separator) {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
        }
        printf(";");
    }
    printf("\n\n");  // Added extra \n

    // Odczyt trzeciej linii
    while (1) {
        val = decode_vbyte(file);
        printf("%d", val);
        
        // Sprawdź czy następny bajt to początek separatora
        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1) {
            if (peek == separator) {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
        }
        printf(";");
    }
    printf("\n\n");  // Added extra \n

    // Odczyt czwartej linii (wierzchołki i ich sąsiedzi)
    while (1) {
        val = decode_vbyte(file);
        printf("%d", val);
        
        // Sprawdź czy następny bajt to początek separatora
        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1) {
            if (peek == separator) {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
            printf(";");
        }
    }
    printf("\n\n");  // Added extra \n

    // Odczyt piątej linii i kolejnych (indeksy dla każdej części)
    while (!feof(file)) {
        val = decode_vbyte(file);
        if (feof(file)) break;
        printf("%d", val);

        // Try to read separator or next byte
        uint64_t peek;
        size_t read_bytes = fread(&peek, sizeof(uint64_t), 1, file);
        
        // If we read separator
        if (read_bytes == 1 && peek == separator) {
            printf("\n\n");
            continue;
        }
        
        // If we read something but it's not a separator
        if (read_bytes == 1) {
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
            printf(";");
            continue;
        }
        
        // If we couldn't read full 8 bytes but not at EOF
        if (read_bytes == 0 && !feof(file)) {
            printf(";");
            continue;
        }
        
        // If we're at EOF but still had a valid number
        if (read_bytes == 0 && feof(file)) {
            break;
        }
    }
    printf("\n");

    fclose(file);
}

void add_neighbor(Node *node, int neighbor)
{
    if (node->neighbor_count == node->neighbor_capacity)
    {
        int new_capacity = (node->neighbor_capacity == 0) ? 2 : node->neighbor_capacity * 2;
        int *new_neighbors = realloc(node->neighbors, new_capacity * sizeof(int));
        if (new_neighbors == NULL)
        {
            perror("Błąd alokacji pamięci dla sąsiadów");
            exit(EXIT_FAILURE);
        }
        node->neighbors = new_neighbors;
        node->neighbor_capacity = new_capacity;
    }

    node->neighbors[node->neighbor_count++] = neighbor;
}

void load_graph(const char *filename, Graph *graph, ParsedData *data)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Nie można otworzyć pliku");
        exit(EXIT_FAILURE);
    }

    // Wczytujemy pierwszą linię (jedna liczba)
    int max_nodes;
    if (fscanf(file, "%d", &max_nodes) != 1)
    {
        perror("Błąd odczytu pierwszej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fgetc(file); // Usuwamy znak nowej linii
    data->line1 = malloc(sizeof(int));
    if (data->line1 == NULL)
    {
        perror("Błąd alokacji pamięci dla line1");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    *(data->line1) = max_nodes;

    // Dynamiczna alokacja bufora dla linii
    size_t bufsize = 128; // Początkowy rozmiar bufora
    char *buffer = malloc(bufsize * sizeof(char));
    if (buffer == NULL)
    {
        perror("Błąd alokacji pamięci dla bufora");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Wczytujemy drugą linię
    ssize_t line_length;
    size_t n = bufsize;
    line_length = getline(&buffer, &n, file);
    if (line_length == -1)
    {
        perror("Błąd odczytu drugiej linii");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie drugiej linii
    data->line2 = NULL;
    data->line2_count = 0;
    char *token = strtok(buffer, ";\n");
    while (token != NULL)
    {
        data->line2 = realloc(data->line2, (data->line2_count + 1) * sizeof(int));
        if (data->line2 == NULL)
        {
            perror("Błąd alokacji pamięci dla line2");
            free(buffer);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line2[data->line2_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    // Wczytujemy trzecią linię
    line_length = getline(&buffer, &n, file);
    if (line_length == -1)
    {
        perror("Błąd odczytu trzeciej linii");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie trzeciej linii
    data->line3 = NULL;
    data->line3_count = 0;
    token = strtok(buffer, ";\n");
    while (token != NULL)
    {
        data->line3 = realloc(data->line3, (data->line3_count + 1) * sizeof(int));
        if (data->line3 == NULL)
        {
            perror("Błąd alokacji pamięci dla line3");
            free(buffer);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line3[data->line3_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    // Wczytujemy czwartą linię: edges
    line_length = getline(&buffer, &n, file);
    if (line_length == -1)
    {
        perror("Brak linii edges");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie linii edges
    data->edges = NULL;
    data->edge_count = 0;
    token = strtok(buffer, ";\n");
    while (token != NULL)
    {
        data->edges = realloc(data->edges, (data->edge_count + 1) * sizeof(int));
        if (data->edges == NULL)
        {
            perror("Błąd alokacji pamięci dla edges");
            free(buffer);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->edges[data->edge_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    // Wczytujemy piątą linię: row_pointers
    line_length = getline(&buffer, &n, file);
    if (line_length == -1)
    {
        perror("Brak linii row_pointers");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie linii row_pointers
    data->row_pointers = NULL;
    data->row_count = 0;
    token = strtok(buffer, ";\n");
    while (token != NULL)
    {
        data->row_pointers = realloc(data->row_pointers, (data->row_count + 1) * sizeof(int));
        if (data->row_pointers == NULL)
        {
            perror("Błąd alokacji pamięci dla row_pointers");
            free(buffer);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->row_pointers[data->row_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    free(buffer); // Zwalniamy bufor
    fclose(file);

    // Tworzenie grafu
    // graph->vertices = data->line2_count;
    // graph->nodes = malloc(graph->vertices * sizeof(Node));
    // if (graph->nodes == NULL)
    // {
    //     perror("Błąd alokacji pamięci dla węzłów grafu");
    //     exit(EXIT_FAILURE);
    // }

    // // Inicjalizacja węzłów
    // for (int i = 0; i < graph->vertices; i++)
    // {
    //     graph->nodes[i].vertex = i;
    //     graph->nodes[i].neighbors = NULL;
    //     graph->nodes[i].neighbor_count = 0;
    //     graph->nodes[i].neighbor_capacity = 0;
    //     graph->nodes[i].part_id = -1; // Ustawienie domyślnej wartości part_id
    // }

    inicialize_graph(graph, data->line2_count);
    // Dodawanie sąsiadów
    for (int i = 0; i < data->row_count; i++)
    {
        int start = data->row_pointers[i];
        int end = (i + 1 < data->row_count) ? data->row_pointers[i + 1] : data->edge_count;

        for (int j = start; j < end; j++)
        {
            int current_vertex = i;
            int neighbor_vertex = data->edges[j];

            if (neighbor_vertex < 0 || neighbor_vertex >= graph->vertices)
            {
                fprintf(stderr, "Nieprawidłowy indeks sąsiada: %d\n", neighbor_vertex);
                continue;
            }

            if (current_vertex != neighbor_vertex)
            {
                add_neighbor(&graph->nodes[current_vertex], neighbor_vertex);
                add_neighbor(&graph->nodes[neighbor_vertex], current_vertex);
            }
        }
    }
}