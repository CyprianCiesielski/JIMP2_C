#include <stdio.h>
#include <assert.h>
#include "partition.h"
#include "graph.h"
#include "file_reader.h"  // for add_neighbor function

// test inicjalizacji struktury partycji
void test_partition_initialization() {
    Partition_data partition_data;
    int parts = 3;
    
    initialize_partition_data(&partition_data, parts);
    
    // sprawdzenie podstawowych wartosci
    assert(partition_data.parts_count == parts && "Nieprawidlowa liczba czesci");
    assert(partition_data.parts != NULL && "Tablica czesci nie zostala zaalokowana");
    
    // sprawdzenie kazdej czesci
    for (int i = 0; i < parts; i++) {
        assert(partition_data.parts[i].part_id == i && "Nieprawidlowe ID czesci");
        assert(partition_data.parts[i].part_vertex_count == 0 && "Poczatkowa liczba wierzcholkow powinna byc 0");
        assert(partition_data.parts[i].capacity == 0 && "Poczatkowa pojemnosc powinna byc 0");
    }
    
    printf("Test inicjalizacji partycji: OK\n");
    free_partition_data(&partition_data, parts);
}

// test dodawania wierzcholkow do partycji
void test_vertex_assignment() {
    Partition_data partition_data;
    int parts = 2;
    
    initialize_partition_data(&partition_data, parts);
    
    // dodaj wierzcholki do pierwszej czesci
    add_partition_data(&partition_data, 0, 1);
    add_partition_data(&partition_data, 0, 2);
    add_partition_data(&partition_data, 0, 3);
    
    // dodaj wierzcholki do drugiej czesci
    add_partition_data(&partition_data, 1, 4);
    add_partition_data(&partition_data, 1, 5);
    
    // sprawdz liczbe wierzcholkow w czesciach
    assert(partition_data.parts[0].part_vertex_count == 3 && "Nieprawidlowa liczba wierzcholkow w pierwszej czesci");
    assert(partition_data.parts[1].part_vertex_count == 2 && "Nieprawidlowa liczba wierzcholkow w drugiej czesci");
    
    // sprawdz czy wierzcholki sa poprawnie przypisane
    assert(partition_data.parts[0].part_vertexes[0] == 1 && "Nieprawidlowy pierwszy wierzcholek w czesci 0");
    assert(partition_data.parts[0].part_vertexes[1] == 2 && "Nieprawidlowy drugi wierzcholek w czesci 0");
    assert(partition_data.parts[0].part_vertexes[2] == 3 && "Nieprawidlowy trzeci wierzcholek w czesci 0");
    assert(partition_data.parts[1].part_vertexes[0] == 4 && "Nieprawidlowy pierwszy wierzcholek w czesci 1");
    assert(partition_data.parts[1].part_vertexes[1] == 5 && "Nieprawidlowy drugi wierzcholek w czesci 1");
    
    printf("Test przypisywania wierzcholkow: OK\n");
    free_partition_data(&partition_data, parts);
}

// test znajdowania sasiadow w partycji
void test_partition_neighbors() {
    Graph graph;
    Partition_data partition_data;
    int parts = 2;
    
    // inicjalizuj struktury
    inicialize_graph(&graph, 6);
    initialize_partition_data(&partition_data, parts);
    
    // stworz prosty graf testowy
    add_neighbor(&graph.nodes[0], 1);
    add_neighbor(&graph.nodes[1], 0);
    add_neighbor(&graph.nodes[1], 2);
    add_neighbor(&graph.nodes[2], 1);
    add_neighbor(&graph.nodes[3], 4);
    add_neighbor(&graph.nodes[4], 3);
    add_neighbor(&graph.nodes[4], 5);
    add_neighbor(&graph.nodes[5], 4);
    
    // przypisz wierzcholki do partycji
    add_partition_data(&partition_data, 0, 0);
    add_partition_data(&partition_data, 0, 1);
    add_partition_data(&partition_data, 0, 2);
    add_partition_data(&partition_data, 1, 3);
    add_partition_data(&partition_data, 1, 4);
    add_partition_data(&partition_data, 1, 5);
    
    // znajdz sasiadow w pierwszej czesci
    int size;
    int **neighbors = get_part_neighbors(&graph, &partition_data, 0, &size);
    
    assert(neighbors != NULL && "Tablica sasiadow nie zostala zaalokowana");
    assert(size == 3 && "Nieprawidlowa liczba wierzcholkow z sasiadami");
    
    // zwolnij pamiec
    for (int i = 0; i < size; i++) {
        free(neighbors[i]);
    }
    free(neighbors);
    free_partition_data(&partition_data, parts);
    free_graph(&graph);
    
    printf("Test znajdowania sasiadow w partycji: OK\n");
}

int main() {
    printf("=== Testy partycji ===\n\n");
    
    test_partition_initialization();
    test_vertex_assignment();
    test_partition_neighbors();
    
    printf("\n=== Wszystkie testy partycji zakonczone ===\n");
    return 0;
}