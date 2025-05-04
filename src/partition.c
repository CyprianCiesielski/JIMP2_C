#include "partition.h"

void initialize_partition_data(Partition_data *partition_data, int parts)
{
    partition_data->parts = malloc(parts * sizeof(Part_data));
    if (partition_data->parts == NULL)
    {
        perror("Błąd alokacji pamięci dla części");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < parts; i++)
    {
        partition_data->parts[i].part_vertexes = NULL;
        partition_data->parts[i].part_vertex_count = NULL;
    }
}
void free_partition_data(Partition_data *partition_data, int parts)
{
    for (int i = 0; i < parts; i++)
    {
        free(partition_data->parts[i].part_vertexes);
        free(partition_data->parts[i].part_vertex_count);
        }
    free(partition_data->parts);
}
void print_partition_data(const Partition_data *partition_data, int parts)
{
    for (int i = 0; i < parts; i++)
    {
        printf("Część %d: ", i);
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++)
        {
            printf("%d ", partition_data->parts[i].part_vertexes[j]);
        }
        printf("\n");
    }
    printf("\n");
    printf("Liczba części: %d\n", parts);
    printf("Liczba wierzchołków w każdej części:\n");
    for (int i = 0; i < parts; i++)
    {
        printf("Część %d: %d\n", i, partition_data->parts[i].part_vertex_count);
    }
    printf("\n");
    printf("Pojemność tablicy w każdej części:\n");
    for (int i = 0; i < parts; i++)
    {
        printf("Część %d: %d\n", i, partition_data->parts[i].capacity);
    }
    printf("\n");
    printf("Wierzchołki w każdej części:\n");
    for (int i = 0; i < parts; i++)
    {
        printf("Część %d: ", i);
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++)
        {
            printf("%d ", partition_data->parts[i].part_vertexes[j]);
        }
        printf("\n");
    }
    printf("\n");
}

void add_partition_data(Partition_data *partition_data, int part_id, int vertex)
{
    // Sprawdzenie, czy część już istnieje
    if (partition_data->parts[part_id].part_vertexes == NULL)
    {
        // Początkowa alokacja z pojemnością dla 8 elementów
        partition_data->parts[part_id].capacity = 128;
        partition_data->parts[part_id].part_vertexes = malloc(partition_data->parts[part_id].capacity * sizeof(int));
        if (partition_data->parts[part_id].part_vertexes == NULL)
        {
            perror("Błąd alokacji pamięci dla wierzchołków części");
            exit(EXIT_FAILURE);
        }
        partition_data->parts[part_id].part_vertex_count = 0;
        partition_data->parts[part_id].part_vertexes[0] = vertex;
        partition_data->parts[part_id].part_vertex_count = 1;
    }
    else
    {
        // Sprawdź, czy potrzebne jest powiększenie tablicy
        if (partition_data->parts[part_id].part_vertex_count >= partition_data->parts[part_id].capacity)
        {
            // Podwojenie pojemności
            partition_data->parts[part_id].capacity *= 2;

            // Realokacja tablicy z nową pojemnością
            int *new_vertexes = realloc(partition_data->parts[part_id].part_vertexes,
                                        partition_data->parts[part_id].capacity * sizeof(int));

            if (new_vertexes == NULL)
            {
                perror("Błąd realokacji pamięci dla wierzchołków części");
                exit(EXIT_FAILURE);
            }

            partition_data->parts[part_id].part_vertexes = new_vertexes;
        }

        // Dodaj nowy wierzchołek
        partition_data->parts[part_id].part_vertexes[partition_data->parts[part_id].part_vertex_count] = vertex;
        partition_data->parts[part_id].part_vertex_count++;
    }
}
