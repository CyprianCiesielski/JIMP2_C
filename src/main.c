#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "graph.h"
#include "file_reader.h"
#include "file_writer.h"
#include "region_growing.h"
#include "partition.h"
#include <time.h>
#include "stats.h"
#include "fm_optimization.h"
#include <math.h>
// wyswietla wszystkie wierzcholki grafu i ich sasiadow
void print_graph(const Graph *graph)
{
    // przejdz przez wszystkie wierzcholki
    for (int i = 0; i < graph->vertices; i++)
    {
        printf("Wierzcholek %d: ", graph->nodes[i].vertex);
        // wypisz sasiadow tego wierzcholka
        for (int j = 0; j < graph->nodes[i].neighbor_count; j++)
        {
            printf("%d, ", graph->nodes[i].neighbors[j]);
        }
        printf("\n");
    }
}

// wyswietla podzial grafu na czesci wraz z sasiadami w kazdej czesci
void print_data(const Graph *graph, const Partition_data *partition_data, int parts)
{
    printf("\nPodzial grafu na %d czesci:\n", parts);
    // dla kazdej czesci
    for (int i = 0; i < parts; i++)
    {
        printf("\nCzesc %d:\n", i);
        // przejdz przez wszystkie wierzcholki w tej czesci
        for (int j = 0; j < partition_data->parts[i].part_vertex_count; j++)
        {
            int current_vertex = partition_data->parts[i].part_vertexes[j];
            printf("Wierzcholek %d: ", current_vertex);

            // policz ile sasiadow ma w tej samej czesci
            int neighbor_count = 0;
            for (int k = 0; k < graph->nodes[current_vertex].neighbor_count; k++)
            {
                int neighbor = graph->nodes[current_vertex].neighbors[k];

                // sprawdz czy sasiad jest w tej samej czesci
                for (int l = 0; l < partition_data->parts[i].part_vertex_count; l++)
                {
                    if (partition_data->parts[i].part_vertexes[l] == neighbor)
                    {
                        if (neighbor_count > 0)
                        {
                            printf(", ");
                        }
                        printf("%d", neighbor);
                        neighbor_count++;
                        break;
                    }
                }
            }
            printf("\n");
        }
    }
}

// wyswietla pomoc programu
void print_usage(char *program_name)
{
    printf("Uzycie: %s [czesci] [dokladnosc] [plik_wejsciowy] [opcje]\n", program_name);
    printf("\nArgumenty pozycyjne:\n");
    printf("  czesci            liczba czesci na ktore dzielic (domyslnie: 2)\n");
    printf("  dokladnosc       dokladnosc podzialu z %% (domyslnie: 10%%)\n");
    printf("  plik_wejsciowy   nazwa pliku wejsciowego (domyslnie: graf.csrrg)\n");
    printf("\nOpcje:\n");
    printf("  --precompute-metrics -p oblicz metryki przed podzialem\n");
    printf("  --statistics -s       wyswietl szczegolowe statystyki\n");
    printf("  --output -o PLIK      nazwa pliku wyjsciowego (domyslnie: anwser.csrrg)\n");
    printf("  --out-format text|binary / -k format wyjsciowy (domyslnie: oba)\n");
    printf("  --force -f            wymus podzial nawet jesli nie spelnia dokladnosci\n");
    printf("  --iterations -i ilosc iteracji funkcji cut_edges_optimalization\n");
    printf("  -h, --help           pokaz ten komunikat pomocy\n");
}

// glowna funkcja programu
int main(int argc, char *argv[])
{
    // domyslne wartosci parametrow
    char path[256];
    char *percent;
    int parts = 2;                   // ilosc czesci
    float accuracy = 0.1;            // dokladnosc podzialu
    char *input_file = "graf.csrrg"; // plik wejsciowy
    char *output_file = "anwser";    // plik wyjsciowy
    int iteration_limit = -1;        // max przecietych krawedzi
    int precompute = 0;              // czy liczyc statystyki
    int force = 0;                   // czy wymusic podzial
    int output_format = 3;           // format wyjsciowy (3=oba)
    int show_statistics = 0;         // czy wyswietlic statystyki

    // sprawdz czy uzytkownik chce pomocy
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
    }

    // parsowanie argumentow pozycyjnych
    int pos_args = 0;
    int i = 1;
    while (i < argc && argv[i][0] != '-')
    {
        switch (pos_args)
        {
        case 0:
            parts = atoi(argv[i]);
            if (parts <= 0)
            {
                perror("liczba czesci musi byc dodatnia");
                return 1;
            }
            break;
        case 1:
            percent = strchr(argv[i], '%');
            if (percent)
                *percent = '\0';
            accuracy = atof(argv[i]) / 100.0;
            if (accuracy <= 0 || accuracy > 1)
            {
                perror("dokladnosc musi byc pomiedzy 1 a 100");
                return 1;
            }
            break;
        case 2:
            input_file = argv[i];
            break;
        }
        pos_args++;
        i++;
    }

    // parsowanie opcji z myslnikiem
    while (i < argc)
    {
        if (strcmp(argv[i], "--precompute-metrics") == 0 || strcmp(argv[i], "-p") == 0)
        {
            precompute = 1;
            i++;
        }
        else if ((strcmp(argv[i], "--output") == 0 && i + 1 < argc) ||
                 (strcmp(argv[i], "-o") == 0 && i + 1 < argc))
        {
            output_file = argv[i + 1];
            i += 2;
        }
        else if ((strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) ||
                 (strcmp(argv[i], "-i") == 0 && i + 1 < argc))
        {
            iteration_limit = atoi(argv[i + 1]);
            if (iteration_limit <= 0)
            {
                perror("ilosc iteracji musi byc dodatnia i w zakresie int");
                return 1;
            }
            i += 2;
        }
        else if ((strcmp(argv[i], "--out-format") == 0 && i + 1 < argc) ||
                 (strcmp(argv[i], "-k") == 0 && i + 1 < argc))
        {
            if (strcmp(argv[i + 1], "text") == 0)
            {
                output_format = 0;
            }
            else if (strcmp(argv[i + 1], "binary") == 0)
            {
                output_format = 1;
            }
            else
            {
                perror("nieznany format wyjsciowy");
                return 1;
            }
            i += 2;
        }
        else if (strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "-f") == 0)
        {
            force = 1;
            i++;
        }
        else if (strcmp(argv[i], "--statistics") == 0 || strcmp(argv[i], "-s") == 0)
        {
            show_statistics = 1;
            i++;
        }
        else
        {
            perror("nieznana opcja");
            perror("uzyj --help aby wyswietlic pomoc");
            return 1;
        }
    }

    // stworz sciezke do pliku wejsciowego
    snprintf(path, sizeof(path), "data/%s", input_file);

    // inicjalizacja struktur i pomiar czasu
    clock_t start = clock();
    Graph graph;
    ParsedData data = {0};
    Partition_data partition_data;

    // wczytaj i przygotuj graf
    printf("Input file: %s\n", path);
    load_graph(path, &graph, &data);
    count_edges(&graph);
    assign_min_max_count(&graph, parts, accuracy);
    printf("Loaded graph with %d vertices and %d edges\n", graph.vertices, graph.edges);

    // zrob wstepny podzial grafu
    printf("Initializing partition data for %d parts\n", parts);
    initialize_partition_data(&partition_data, parts);

    // glowny algorytm podzialu
    int success = region_growing(&graph, parts, &partition_data, accuracy);
    if (!success && !force)
    {
        perror("nie udalo sie osiagnac zadanej dokladnosci, uzyj --force aby wymusic");
        free_graph(&graph);
        free_partition_data(&partition_data, parts);
        return 1;
    }

    // optymalizacja podzialu
    printf("\nOptimizing with Fiduccia-Mattheyses algorithm...\n");
    cut_edges_optimization(&graph, &partition_data, iteration_limit > 0 ? iteration_limit : 1000);

    // sprawdz spojnosc
    check_partition_connectivity(&graph, parts);

    // przygotuj nazwy plikow wyjsciowych
    char output_path[256];
    char binary_path[256];
    snprintf(output_path, sizeof(output_path), "data/%s.csrrg", output_file);
    snprintf(binary_path, sizeof(binary_path), "data/%s.bin", output_file);

    // zapisz wyniki w odpowiednim formacie
    if (output_format == 3) // zapisz oba
    {
        write_text(output_path, &data, &partition_data, &graph, parts);
        write_binary(binary_path, &data, &partition_data, &graph, parts);
    }
    else if (output_format == 0) // tylko tekst
    {
        write_text(output_path, &data, &partition_data, &graph, parts);
    }
    else if (output_format == 1) // tylko binarny
    {
        write_binary(binary_path, &data, &partition_data, &graph, parts);
    }

    // policz czas wykonania
    clock_t end = clock();
    double execution_time = (double)(end - start) / CLOCKS_PER_SEC;

    // wyswietl statystyki jesli trzeba
    if (show_statistics)
    {
        print_statistics(&graph, &partition_data, parts, accuracy, precompute, execution_time);
    }

    if (precompute)
    {
        print_precompute_metrics(&graph, &partition_data, parts);
    }

    // pokaz sasiadow pierwszej czesci
    print_part_neighbors(get_part_neighbors(&graph, &partition_data, 0, NULL), 0);

    // posprzataj
    free_graph(&graph);
    free_partition_data(&partition_data, parts);

    return 0;
}