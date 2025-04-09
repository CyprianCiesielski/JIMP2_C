#include "file_reader.h"

//strtok

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

void load_graph(const char *filename, Graph *graph) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Nie można otworzyć pliku");
        exit(EXIT_FAILURE);
    }

    char buffer[10000];

    // Pomijamy pierwszą linię
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Błąd odczytu pierwszej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Wczytujemy drugą linię
    char second_line[10000];
    if (fgets(second_line, sizeof(second_line), file) == NULL) {
        perror("Błąd odczytu drugiej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Pomijamy trzecią linię
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        perror("Błąd odczytu trzeciej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Liczenie liczb w drugiej linii
    int count = 0;
    char *tokeni = strtok(second_line, ";\n");
    while (tokeni != NULL) {
        count++;
        tokeni = strtok(NULL, ";\n");
    }

    // Wczytujemy czwartą linię: edges
    char edges_line[10000];
    if (fgets(edges_line, sizeof(edges_line), file) == NULL) {
        perror("Brak linii edges");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Wczytujemy piątą linię: row_pointers
    char row_pointers_line[10000];
    if (fgets(row_pointers_line, sizeof(row_pointers_line), file) == NULL) {
        perror("Brak linii row_pointers");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);

    // Parsowanie linii edges
    int edge_count = 0;
    char *edges_dup = strdup(edges_line);
    char *token = strtok(edges_dup, ";\n");
    while (token != NULL) {
        edge_count++;
        token = strtok(NULL, ";\n");
    }
    free(edges_dup);

    int *edges = malloc(edge_count * sizeof(int));
    if (edges == NULL) {
        perror("Błąd alokacji pamięci dla edges");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    edges_dup = strdup(edges_line);
    token = strtok(edges_dup, ";\n");
    while (token != NULL) {
        edges[index++] = atoi(token);
        token = strtok(NULL, ";\n");
    }
    free(edges_dup);

    // Parsowanie linii row_pointers
    int row_count = 0;
    char *rp_dup = strdup(row_pointers_line);
    token = strtok(rp_dup, ";\n");
    while (token != NULL) {
        row_count++;
        token = strtok(NULL, ";\n");
    }
    free(rp_dup);

    int *row_pointers = malloc(row_count * sizeof(int));
    if (row_pointers == NULL) {
        perror("Błąd alokacji pamięci dla row_pointers");
        free(edges);
        exit(EXIT_FAILURE);
    }

    index = 0;
    rp_dup = strdup(row_pointers_line);
    token = strtok(rp_dup, ";\n");
    while (token != NULL) {
        row_pointers[index++] = atoi(token);
        token = strtok(NULL, ";\n");
    }
    free(rp_dup);
    
    // Tworzenie grafu
    graph->vertices = count;
    graph->nodes = malloc(graph->vertices * sizeof(Node));
    if (graph->nodes == NULL) {
        perror("Błąd alokacji pamięci dla węzłów grafu");
        free(edges);
        free(row_pointers);
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
    for (int i = 0; i < row_count; i++) {
        int start = row_pointers[i];
        int end = (i + 1 < row_count) ? row_pointers[i + 1] : edge_count;

        for (int j = start; j < end; j++) {
            int current_vertex = i;
            int neighbor_vertex = edges[j];

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

    free(edges);
    free(row_pointers);
}