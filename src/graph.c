#include "graph.h"

// funkcja wypisuje sasiadow kazdego wierzcholka
// przyjmuje tablice sasiedztwa i jej rozmiar
void print_part_neighbors(int **neighbors, int size)
{
    // sprawdzam czy dane wejsciowe sa poprawne
    if (!neighbors || size <= 0)
    {
        return;
    }

    // iteruje po wszystkich wierzcholkach
    for (int i = 0; i < size; i++)
    {
        // pomijam jesli wierzcholek nie ma przypisanych sasiadow
        if (!neighbors[i])
        {
            continue;
        }
        printf("Wierzchołek %d: ", neighbors[i][0]);
        int j = 1;
        // wypisuje wszystkich sasiadow wierzcholka, koncza sie wartoscia -1
        while (neighbors[i][j] != -1)
        {
            printf("%d", neighbors[i][j]);
            // dodaje przecinek jesli to nie ostatni sasiad
            if (neighbors[i][j + 1] != -1)
            {
                printf(", ");
            }
            j++;
        }
        printf("\n");
    }
}

// inicjalizuje strukture grafu z podana liczba wierzcholkow
// alokuje pamiec i ustawia wartosci poczatkowe
void inicialize_graph(Graph *graph, int vertices)
{
    // ustawiam podstawowe parametry grafu
    graph->vertices = vertices;
    graph->edges = 0;
    graph->parts = 0;
    graph->min_count = 0;
    graph->max_count = 0;

    // alokuje pamiec na wezly grafu
    graph->nodes = malloc(vertices * sizeof(Node));
    if (graph->nodes == NULL)
    {
        perror("Blad alokacji pamieci dla wezlow");
        exit(EXIT_FAILURE);
    }
    // inicjalizuje kazdy wezel
    for (int i = 0; i < vertices; i++)
    {
        graph->nodes[i].vertex = i;
        graph->nodes[i].neighbors = NULL;
        graph->nodes[i].neighbor_count = 0;
        graph->nodes[i].part_id = -1;
        graph->nodes[i].neighbor_capacity = 0;
    }
}

// liczy liczbe krawedzi w grafie
// sumuje sasiadow i dzieli przez 2 (bo kazda krawedz jest liczona 2 razy)
void count_edges(Graph *graph)
{
    int edge_count = 0;
    // zliczam wszystkie polaczenia w grafie
    for (int i = 0; i < graph->vertices; i++)
    {
        edge_count += graph->nodes[i].neighbor_count;
    }
    // dziele przez 2 bo kazda krawedz jest liczona podwojnie
    graph->edges = edge_count / 2;
}

// przypisuje minimalna i maksymalna liczbe wierzcholkow na czesc
// na podstawie liczby czesci i parametru dokladnosci
void assign_min_max_count(Graph *graph, int parts, float accuracy)
{
    // sprawdzam czy parametry sa poprawne
    if (parts <= 0 || accuracy < 0.0 || accuracy > 1.0)
    {
        perror("Niepoprawna liczba czesci lub wartosc dokladnosci");
        return;
    }

    graph->parts = parts;

    // licze srednia liczbe wierzcholkow na czesc
    float avg_vertices_per_part = (float)graph->vertices / parts;

    // obliczam min i max z uwzglednieniem dokladnosci
    int min_vertices_per_part = (int)((avg_vertices_per_part * (1.0 - accuracy)));
    // zaokraglam w gore jesli potrzeba
    if ((float)min_vertices_per_part < (avg_vertices_per_part * (1.0 - accuracy)))
    {
        min_vertices_per_part += 1;
    }
    int max_vertices_per_part = (int)(avg_vertices_per_part * (1.0 + accuracy));
    // zapisuje wyniki
    graph->min_count = min_vertices_per_part;
    graph->max_count = max_vertices_per_part;
}

// prosta funkcja ustawiajaca liczbe czesci grafu
void assing_parts(Graph *graph, int parts)
{
    graph->parts = parts;
}

// zwalnia pamiec zaalokowana dla grafu
void free_graph(Graph *graph)
{
    // zwalniam pamiec dla kazdego wezla
    for (int i = 0; i < graph->vertices; i++)
    {
        free(graph->nodes[i].neighbors);
    }
    // zwalniam pamiec dla tablicy wezlow
    free(graph->nodes);
}