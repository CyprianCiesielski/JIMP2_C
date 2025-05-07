#include "file_reader.h"
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

int MAX_BUFFOR = INT_MAX;
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

void read_binary(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nie można otworzyć pliku binarnego do odczytu");
        exit(EXIT_FAILURE);
    }

    const uint64_t separator = 0xDEADBEEFCAFEBABE;
    uint64_t read_separator;

    int num_vertices = decode_vbyte(file);
    printf("%d\n\n", num_vertices);

    fread(&read_separator, sizeof(uint64_t), 1, file);
    if (read_separator != separator)
    {
        fprintf(stderr, "Błąd: brak separatora po pierwszej linii\n");
        fclose(file);
        return;
    }

    int val;
    while (1)
    {
        val = decode_vbyte(file);
        printf("%d", val);

        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1)
        {
            if (peek == separator)
            {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
        }
        printf(";");
    }
    printf("\n\n");

    while (1)
    {
        val = decode_vbyte(file);
        printf("%d", val);

        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1)
        {
            if (peek == separator)
            {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
        }
        printf(";");
    }
    printf("\n\n");

    while (1)
    {
        val = decode_vbyte(file);
        printf("%d", val);

        uint64_t peek;
        if (fread(&peek, sizeof(uint64_t), 1, file) == 1)
        {
            if (peek == separator)
            {
                break;
            }
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
            printf(";");
        }
    }
    printf("\n\n");

    while (!feof(file))
    {
        val = decode_vbyte(file);
        if (feof(file))
            break;
        printf("%d", val);

        uint64_t peek;
        size_t read_bytes = fread(&peek, sizeof(uint64_t), 1, file);

        if (read_bytes == 1 && peek == separator)
        {
            printf("\n\n");
            continue;
        }

        if (read_bytes == 1)
        {
            fseek(file, -sizeof(uint64_t), SEEK_CUR);
            printf(";");
            continue;
        }

        if (read_bytes == 0 && !feof(file))
        {
            printf(";");
            continue;
        }

        if (read_bytes == 0 && feof(file))
        {
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

    int max_nodes;
    if (fscanf(file, "%d", &max_nodes) != 1)
    {
        perror("Błąd odczytu pierwszej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fgetc(file);
    data->line1 = malloc(sizeof(int));
    if (data->line1 == NULL)
    {
        perror("Błąd alokacji pamięci dla line1");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    *(data->line1) = max_nodes;
    printf("Wczytano pierwszą linię:\n");

    char *buffer = malloc(MAX_BUFFOR * sizeof(char));
    if (!buffer)
    {
        perror("Failed to allocate buffer");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("Failed to read second line");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    data->line2 = NULL;
    data->line2_count = 0;
    char *token = strtok(buffer, ";");
    while (token)
    {
        int *new_line2 = realloc(data->line2, (data->line2_count + 1) * sizeof(int));
        if (!new_line2)
        {
            perror("Failed to reallocate line2");
            free(buffer);
            free(data->line2);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line2 = new_line2;
        data->line2[data->line2_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("Failed to read third line");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    data->line3 = NULL;
    data->line3_count = 0;
    token = strtok(buffer, ";");
    while (token)
    {
        int *new_line3 = realloc(data->line3, (data->line3_count + 1) * sizeof(int));
        if (!new_line3)
        {
            perror("Failed to reallocate line3");
            free(buffer);
            free(data->line3);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line3 = new_line3;
        data->line3[data->line3_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("Failed to read edges line");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    data->edges = NULL;
    data->edge_count = 0;
    token = strtok(buffer, ";");
    while (token)
    {
        int *new_edges = realloc(data->edges, (data->edge_count + 1) * sizeof(int));
        if (!new_edges)
        {
            perror("Failed to reallocate edges");
            free(buffer);
            free(data->edges);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->edges = new_edges;
        data->edges[data->edge_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("Failed to read row_pointers line");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    data->row_pointers = NULL;
    data->row_count = 0;
    token = strtok(buffer, ";");
    while (token)
    {
        int *new_row_pointers = realloc(data->row_pointers, (data->row_count + 1) * sizeof(int));
        if (!new_row_pointers)
        {
            perror("Failed to reallocate row_pointers");
            free(buffer);
            free(data->row_pointers);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->row_pointers = new_row_pointers;
        data->row_pointers[data->row_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    free(buffer);
    fclose(file);
 
    
    printf("%d\n", data->line2_count);
    inicialize_graph(graph, data->line2_count);
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