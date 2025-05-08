#include "file_reader.h"
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

// stale i zmienne globalne
int MAX_BUFFOR = INT_MAX; // maksymalny rozmiar bufora do czytania pliku

// czyta liczbe zakodowana w formacie vbyte z pliku binarnego
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

// czyta i wyswietla zawartosc pliku binarnego
void read_binary(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("nie mozna otworzyc pliku binarnego");
        exit(EXIT_FAILURE);
    }

    const uint64_t separator = 0xDEADBEEFCAFEBABE;
    uint64_t read_separator;

    int num_vertices = decode_vbyte(file);
    printf("%d\n\n", num_vertices);

    if (read_separator != separator)
    {
        perror("brak separatora po pierwszej linii");
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

// dodaje sasiada do listy sasiadow wierzcholka
void add_neighbor(Node *node, int neighbor)
{
    if (node->neighbor_count == node->neighbor_capacity)
    {
        int new_capacity = (node->neighbor_capacity == 0) ? 2 : node->neighbor_capacity * 2;
        int *new_neighbors = realloc(node->neighbors, new_capacity * sizeof(int));
        if (new_neighbors == NULL)
        {
            perror("blad alokacji pamieci dla sasiadow");
            exit(EXIT_FAILURE);
        }
        node->neighbors = new_neighbors;
        node->neighbor_capacity = new_capacity;
    }

    node->neighbors[node->neighbor_count++] = neighbor;
}

// wczytuje graf z pliku tekstowego
void load_graph(const char *filename, Graph *graph, ParsedData *data)
{
    // otworz plik
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("nie mozna otworzyc pliku");
        exit(EXIT_FAILURE);
    }

    // wczytaj liczbe wierzcholkow (pierwsza linia)
    int max_nodes;
    if (fscanf(file, "%d", &max_nodes) != 1)
    {
        perror("blad przy czytaniu pierwszej linii");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fgetc(file); // zjedz znak nowej linii

    // zaalokuj pamiec na liczbe wierzcholkow
    data->line1 = malloc(sizeof(int));
    if (data->line1 == NULL)
    {
        perror("brak pamieci na line1");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    *(data->line1) = max_nodes;

    // wczytaj druga linie (wskazniki do wierszy)
    char *buffer = malloc(MAX_BUFFOR * sizeof(char));
    if (!buffer)
    {
        perror("brak pamieci na bufor");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // wczytaj linie do bufora
    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("nie udalo sie wczytac linii");
        free(buffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // usun znak nowej linii z konca
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    // parsuj liczby z drugiej linii
    data->line2 = NULL;
    data->line2_count = 0;
    char *token = strtok(buffer, ";");
    while (token)
    {
        // powiekszaj tablice dynamicznie
        int *new_line2 = realloc(data->line2, (data->line2_count + 1) * sizeof(int));
        if (!new_line2)
        {
            perror("brak pamieci na line2");
            free(buffer);
            free(data->line2);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line2 = new_line2;
        data->line2[data->line2_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    // wczytaj trzecia linie (liczby sasiadow)
    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("nie udalo sie wczytac linii");
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
            perror("nie udalo sie wczytac linii");
            free(buffer);
            free(data->line3);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->line3 = new_line3;
        data->line3[data->line3_count++] = atoi(token);
        token = strtok(NULL, ";");
    }

    // wczytaj czwarta linie (krawedzie)
    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("nie udalo sie wczytac linii");
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
            perror("blad alokacji pamieci dla krawedzi");
            free(buffer);
            free(data->edges);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        data->edges = new_edges;
        data->edge_count++;
        data->edges[data->edge_count - 1] = atoi(token);
        token = strtok(NULL, ";");
    }

    // wczytaj piata linie (wskazniki do wierszy)
    if (!fgets(buffer, MAX_BUFFOR, file))
    {
        perror("nie udalo sie wczytac linii");
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
            perror("blad alokacji pamieci dla wskaznikow wierszy");
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

    // stworz graf i dodaj krawedzie
    // printf("Tworzenie grafu z %d wierzcholkami\n", data->line2_count);
    inicialize_graph(graph, data->line2_count);

    // dodaj wszystkie krawedzie do grafu
    for (int i = 0; i < data->row_count; i++)
    {
        // ustal zakres sasiadow dla biezacego wierzcholka
        int start = data->row_pointers[i];
        int end = (i + 1 < data->row_count) ? data->row_pointers[i + 1] : data->edge_count;

        // dodaj wszystkich sasiadow
        for (int j = start; j < end; j++)
        {
            int current_vertex = i;
            int neighbor_vertex = data->edges[j];

            // sprawdz poprawnosc indeksu sasiada
            if (neighbor_vertex < 0 || neighbor_vertex >= graph->vertices)
            {
                perror("niepoprawny indeks sasiada");
                continue;
            }

            // dodaj krawedz w obie strony bo graf jest nieskierowany
            if (current_vertex != neighbor_vertex)
            {
                add_neighbor(&graph->nodes[current_vertex], neighbor_vertex);
                add_neighbor(&graph->nodes[neighbor_vertex], current_vertex);
            }
        }
    }
}