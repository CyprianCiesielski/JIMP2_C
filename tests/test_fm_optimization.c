#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "graph.h"
#include "partition.h"
#include "fm_optimization.h"

// pomocnicza funkcja do tworzenia prostego grafu testowego
Graph *create_test_graph(int vertices, int parts)
{
    Graph *graph = malloc(sizeof(Graph));
    graph->vertices = vertices;
    graph->parts = parts;
    graph->nodes = malloc(vertices * sizeof(Node));
    graph->min_count = 1;        // minimalna liczba wierzchołków w partycji
    graph->max_count = vertices; // maksymalna liczba wierzchołków

    // inicjalizacja wierzchołków
    for (int i = 0; i < vertices; i++)
    {
        graph->nodes[i].vertex = i;
        graph->nodes[i].neighbor_count = 0;
        graph->nodes[i].neighbors = malloc(vertices * sizeof(int)); // zapas miejsca
        graph->nodes[i].part_id = i % parts;                        // równy podział na partie
    }

    return graph;
}

// dodawanie krawędzi do grafu testowego
void add_edge(Graph *graph, int from, int to)
{
    // sprawdź czy krawędź nie istnieje już
    for (int i = 0; i < graph->nodes[from].neighbor_count; i++)
    {
        if (graph->nodes[from].neighbors[i] == to)
            return;
    }

    // dodaj krawędź w obu kierunkach
    graph->nodes[from].neighbors[graph->nodes[from].neighbor_count++] = to;
    graph->nodes[to].neighbors[graph->nodes[to].neighbor_count++] = from;
}

// zwolnij zasoby grafu
void free_test_graph(Graph *graph)
{
    for (int i = 0; i < graph->vertices; i++)
    {
        free(graph->nodes[i].neighbors);
    }
    free(graph->nodes);
    free(graph);
}

// test funkcji is_partition_connected
void test_is_partition_connected()
{
    printf("Test: is_partition_connected\n");

    // Tworzymy graf z 6 wierzchołkami i 2 partiami
    Graph *graph = create_test_graph(6, 2);

    // Ustawiamy wszystkie wierzchołki do partycji 0
    for (int i = 0; i < 6; i++)
    {
        graph->nodes[i].part_id = 0;
    }

    // Dodajemy krawędzie tworząc spójny podgraf
    add_edge(graph, 0, 1);
    add_edge(graph, 1, 2);
    add_edge(graph, 2, 3);
    add_edge(graph, 3, 4);
    add_edge(graph, 4, 5);

    // Sprawdzamy czy partycja jest spójna
    assert(is_partition_connected(graph, 0) == 1);

    // Zmieniamy wierzchołek 3 na partycję 1, rozdzielając partycję 0
    graph->nodes[3].part_id = 1;

    // Teraz partycja 0 nie powinna być spójna
    assert(is_partition_connected(graph, 0) == 0);

    printf("OK\n");
    free_test_graph(graph);
}

// test funkcji will_remain_connected_if_removed
void test_will_remain_connected_if_removed()
{
    printf("Test: will_remain_connected_if_removed\n");

    // Tworzymy graf z 5 wierzchołkami w jednej partycji
    Graph *graph = create_test_graph(5, 1);

    // Dodajemy krawędzie tworząc ścieżkę 0-1-2-3-4
    add_edge(graph, 0, 1);
    add_edge(graph, 1, 2);
    add_edge(graph, 2, 3);
    add_edge(graph, 3, 4);

    // Usunięcie wierzchołka 2 powinno zepsuć spójność
    assert(will_remain_connected_if_removed(graph, 2) == 0);

    // Dodajemy krawędź omijającą wierzchołek 2
    add_edge(graph, 1, 3);

    // Teraz usunięcie wierzchołka 2 nie powinno zepsuć spójności
    assert(will_remain_connected_if_removed(graph, 2) == 1);

    printf("OK\n");
    free_test_graph(graph);
}

// test funkcji calculate_gain
void test_calculate_gain()
{
    printf("Test: calculate_gain\n");

    // Tworzymy graf z 4 wierzchołkami i 2 partiami
    Graph *graph = create_test_graph(4, 2);

    // Wierzchołki 0,1 w partycji 0, wierzchołki 2,3 w partycji 1
    graph->nodes[0].part_id = 0;
    graph->nodes[1].part_id = 0;
    graph->nodes[2].part_id = 1;
    graph->nodes[3].part_id = 1;

    // Dodajemy krawędzie
    add_edge(graph, 0, 1); // wewnątrz partycji 0
    add_edge(graph, 0, 2); // między partycjami
    add_edge(graph, 2, 3); // wewnątrz partycji 1

    // Inicjalizujemy kontekst FM
    Partition_data *partition_data = malloc(sizeof(Partition_data));
    partition_data->parts = malloc(2 * sizeof(Part));
    partition_data->parts[0].part_vertex_count = 2;
    partition_data->parts[1].part_vertex_count = 2;

    FM_Context *context = initialize_fm_context(graph, partition_data, 1);

    // Przeniesienie wierzchołka 0 do partycji 1:
    // - tracimy krawędź wewnątrz partycji 0 (0-1)
    // - zyskujemy krawędź wewnątrz nowej partycji (0-2)
    // Oczekiwany zysk: +1 -1 = 0
    assert(calculate_gain(context, 0, 1) == 0);

    // Przeniesienie wierzchołka 2 do partycji 0:
    // - tracimy krawędź wewnątrz partycji 1 (2-3)
    // - zyskujemy krawędź wewnątrz nowej partycji (0-2)
    // Oczekiwany zysk: +1 -1 = 0
    assert(calculate_gain(context, 2, 0) == 0);

    // Dodajemy dodatkową krawędź między partycjami
    add_edge(graph, 1, 2);

    // Teraz przeniesienie wierzchołka 2 do partycji 0:
    // - tracimy krawędź wewnątrz partycji 1 (2-3)
    // - zyskujemy 2 krawędzie wewnątrz nowej partycji (0-2, 1-2)
    // Oczekiwany zysk: +2 -1 = 1
    assert(calculate_gain(context, 2, 0) == 1);

    printf("OK\n");
    free_fm_context(context);
    free(partition_data->parts);
    free(partition_data);
    free_test_graph(graph);
}

// test funkcji is_valid_move
void test_is_valid_move()
{
    printf("Test: is_valid_move\n");

    // Tworzymy graf z 4 wierzchołkami i 2 partiami
    Graph *graph = create_test_graph(4, 2);
    graph->min_count = 1; // minimalna liczba wierzchołków w partycji
    graph->max_count = 3; // maksymalna liczba wierzchołków

    // Wierzchołki 0,1 w partycji 0, wierzchołki 2,3 w partycji 1
    graph->nodes[0].part_id = 0;
    graph->nodes[1].part_id = 0;
    graph->nodes[2].part_id = 1;
    graph->nodes[3].part_id = 1;

    // Inicjalizujemy kontekst FM
    Partition_data *partition_data = malloc(sizeof(Partition_data));
    partition_data->parts = malloc(2 * sizeof(Part));
    partition_data->parts[0].part_vertex_count = 2;
    partition_data->parts[1].part_vertex_count = 2;

    FM_Context *context = initialize_fm_context(graph, partition_data, 1);

    // Wszystkie ruchy powinny być dozwolone
    assert(is_valid_move(context, 0, 1) == 1);
    // assert(is_valid_move(context, 1, 1) == 0); // ten sam part_id, działa
    assert(is_valid_move(context, 2, 0) == 1);

    // Blokujemy wierzchołek 0
    context->locked[0] = true;
    assert(is_valid_move(context, 0, 1) == 0);

    printf("OK\n");
    free_fm_context(context);
    free(partition_data->parts);
    free(partition_data);
    free_test_graph(graph);
}

// główna funkcja testująca
int main()
{
    printf("====== Testy FM Optimization ======\n");

    test_is_partition_connected();
    test_will_remain_connected_if_removed();
    test_calculate_gain();
    test_is_valid_move();

    printf("\nWszystkie testy zakończone pomyślnie!\n");
    return 0;
}