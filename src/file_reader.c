#include "file_reader.h"

// Funkcja pomocnicza do dekodowania liczby w formacie vByte
int decode_vbyte(FILE *file) {
    int value = 0;
    int shift = 0;
    uint8_t byte;

    while (fread(&byte, sizeof(uint8_t), 1, file) == 1) {
        value |= (byte & 0x7F) << shift; // Dodaj 7 bitów do wartości
        if ((byte & 0x80) == 0) {       // Jeśli MSB = 0, to koniec liczby
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

    // Separator w formacie little-endian
    const uint64_t separator = 0xDEADBEEFCAFEBABE;
    uint64_t read_separator;

    // Odczyt pierwszej linii (liczba wierzchołków)
    int num_vertices = decode_vbyte(file);
    printf("%d\n", num_vertices);

    // Odczyt separatora między pierwszym a drugim wierszem
    fread(&read_separator, sizeof(uint64_t), 1, file);
    if (read_separator != separator) {
        fprintf(stderr, "Błąd: brak separatora po pierwszym wierszu\n");
        fclose(file);
        return;
    }

    // Odczyt drugiej linii (lista sąsiadów)
    printf("Lista sąsiadów:\n");
    while (1) {
        if (fread(&read_separator, sizeof(uint64_t), 1, file) == 1 && read_separator == separator) {
            break; // Separator oznacza koniec sekcji lista sąsiadów
        }

        fseek(file, -sizeof(uint64_t), SEEK_CUR); // Cofnij wskaźnik pliku
        int value = decode_vbyte(file);
        printf("%d;", value);
    }
    printf("\n");

    // Odczyt trzeciej linii (row_pointers)
    printf("Row pointers:\n");
    while (1) {
        if (fread(&read_separator, sizeof(uint64_t), 1, file) == 1 && read_separator == separator) {
            break; // Separator oznacza koniec sekcji row_pointers
        }

        fseek(file, -sizeof(uint64_t), SEEK_CUR); // Cofnij wskaźnik pliku
        int value = decode_vbyte(file);
        printf("%d;", value);
    }
    printf("\n");

    // Odczyt czwartej linii (krawędzie)
    printf("Krawędzie:\n");
    while (!feof(file)) {
        int value = decode_vbyte(file);
        if (feof(file)) {
            break;
        }
        printf("%d;", value);
    }
    printf("\n");

    fclose(file);
}

void add_neighbor(Node *node, int neighbor) {
    if (node->neighbor_count == node->neighbor_capacity) {
        int new_capacity = (node->neighbor_capacity == 0) ? 2 : node->neighbor_capacity * 2;
        int *new_neighbors = realloc(node->neighbors, new_capacity * sizeof(int));
        if (new_neighbors == NULL) {
            perror("Błąd alokacji pamięci dla sąsiadów");
            exit(EXIT_FAILURE);
        }
        node->neighbors = new_neighbors;
        node->neighbor_capacity = new_capacity;
    }

    node->neighbors[node->neighbor_count++] = neighbor;
}

void load_graph(const char *filename, Graph *graph, ParsedData *data) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Nie można otworzyć pliku");
        exit(EXIT_FAILURE);
    }

    // Wczytujemy pierwszą linię (jedna liczba)
    int max_nodes;
    if (fscanf(file, "%d", &max_nodes) != 1) {
        perror("Błąd odczytu pierwszej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fgetc(file); // Usuwamy znak nowej linii
    data->line1 = malloc(sizeof(int));
    if (data->line1 == NULL) {
        perror("Błąd alokacji pamięci dla line1");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    *(data->line1) = max_nodes;

    // Wczytujemy drugą linię
    char second_line[10000];
    if (fgets(second_line, sizeof(second_line), file) == NULL) {
        perror("Błąd odczytu drugiej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie drugiej linii
    data->line2 = NULL;
    data->line2_count = 0;
    char *token = strtok(second_line, ";\n");
    while (token != NULL) {
        data->line2 = realloc(data->line2, (data->line2_count + 1) * sizeof(int));
        if (data->line2 == NULL) {
            perror("Błąd alokacji pamięci dla line2");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line2[data->line2_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    // Wczytujemy trzecią linię
    char third_line[10000];
    if (fgets(third_line, sizeof(third_line), file) == NULL) {
        perror("Błąd odczytu trzeciej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie trzeciej linii
    data->line3 = NULL;
    data->line3_count = 0;
    char *third_line_dup = strdup(third_line);
    if (third_line_dup == NULL) {
        perror("Błąd alokacji pamięci dla third_line_dup");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    token = strtok(third_line_dup, ";\n");
    while (token != NULL) {
        data->line3 = realloc(data->line3, (data->line3_count + 1) * sizeof(int));
        if (data->line3 == NULL) {
            perror("Błąd alokacji pamięci dla line3");
            free(third_line_dup);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line3[data->line3_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }
    free(third_line_dup);

    // Wczytujemy czwartą linię: edges
    char edges_line[10000];
    if (fgets(edges_line, sizeof(edges_line), file) == NULL) {
        perror("Brak linii edges");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie linii edges
    data->edges = NULL;
    data->edge_count = 0;
    token = strtok(edges_line, ";\n");
    while (token != NULL) {
        data->edges = realloc(data->edges, (data->edge_count + 1) * sizeof(int));
        if (data->edges == NULL) {
            perror("Błąd alokacji pamięci dla edges");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->edges[data->edge_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    // Wczytujemy piątą linię: row_pointers
    char row_pointers_line[10000];
    if (fgets(row_pointers_line, sizeof(row_pointers_line), file) == NULL) {
        perror("Brak linii row_pointers");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Parsowanie linii row_pointers
    data->row_pointers = NULL;
    data->row_count = 0;
    token = strtok(row_pointers_line, ";\n");
    while (token != NULL) {
        data->row_pointers = realloc(data->row_pointers, (data->row_count + 1) * sizeof(int));
        if (data->row_pointers == NULL) {
            perror("Błąd alokacji pamięci dla row_pointers");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->row_pointers[data->row_count++] = atoi(token);
        token = strtok(NULL, ";\n");
    }

    fclose(file);

    // Tworzenie grafu
    graph->vertices = data->line2_count;
    graph->nodes = malloc(graph->vertices * sizeof(Node));
    if (graph->nodes == NULL) {
        perror("Błąd alokacji pamięci dla węzłów grafu");
        free(data->edges);
        free(data->row_pointers);
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja węzłów
    for (int i = 0; i < graph->vertices; i++) {
        graph->nodes[i].vertex = i;
        graph->nodes[i].neighbors = NULL;
        graph->nodes[i].neighbor_count = 0;
        graph->nodes[i].neighbor_capacity = 0;
    }

    // Dodawanie sąsiadów
    for (int i = 0; i < data->row_count; i++) {
        int start = data->row_pointers[i];
        int end = (i + 1 < data->row_count) ? data->row_pointers[i + 1] : data->edge_count;

        for (int j = start; j < end; j++) {
            int current_vertex = i;
            int neighbor_vertex = data->edges[j];

            if (neighbor_vertex < 0 || neighbor_vertex >= graph->vertices) {
                fprintf(stderr, "Nieprawidłowy indeks sąsiada: %d\n", neighbor_vertex);
                continue;
            }

            if (current_vertex != neighbor_vertex) {
                add_neighbor(&graph->nodes[current_vertex], neighbor_vertex);
                add_neighbor(&graph->nodes[neighbor_vertex], current_vertex);
            }
        }
    }
}