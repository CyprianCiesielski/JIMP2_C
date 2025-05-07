#include <stdio.h>
#include <assert.h>
#include "graph.h"
#include "file_reader.h"

// test inicjalizacji grafu
void test_graph_initialization() {
    Graph graph;
    int test_vertices = 5;
    
    // inicjalizacja grafu
    inicialize_graph(&graph, test_vertices);
    
    // sprawdzenie podstawowych wartosci
    assert(graph.vertices == test_vertices && "Nieprawidlowa liczba wierzcholkow");
    assert(graph.edges == 0 && "Liczba krawedzi powinna byc 0 po inicjalizacji");
    assert(graph.parts == 0 && "Liczba czesci powinna byc 0 po inicjalizacji");
    assert(graph.nodes != NULL && "Tablica nodes nie zostala zaalokowana");
    
    // sprawdzenie wierzcholkow
    for (int i = 0; i < test_vertices; i++) {
        assert(graph.nodes[i].vertex == i && "Nieprawidlowy indeks wierzcholka");
        assert(graph.nodes[i].neighbor_count == 0 && "Poczatkowa liczba sasiadow powinna byc 0");
        assert(graph.nodes[i].part_id == -1 && "Poczatkowe ID partycji powinno byc -1");
    }
    
    printf("Test inicjalizacji grafu: OK\n");
    free_graph(&graph);
}

// test dodawania krawedzi do grafu
void test_edge_operations() {
    Graph graph;
    inicialize_graph(&graph, 4);
    
    // dodaj krawedzie 0-1, 1-2, 2-3 (sciezka)
    add_neighbor(&graph.nodes[0], 1);
    add_neighbor(&graph.nodes[1], 0);
    add_neighbor(&graph.nodes[1], 2);
    add_neighbor(&graph.nodes[2], 1);
    add_neighbor(&graph.nodes[2], 3);
    add_neighbor(&graph.nodes[3], 2);
    
    // sprawdz liczbe sasiadow
    assert(graph.nodes[0].neighbor_count == 1 && "Wierzcholek 0 powinien miec 1 sasiada");
    assert(graph.nodes[1].neighbor_count == 2 && "Wierzcholek 1 powinien miec 2 sasiadow");
    assert(graph.nodes[2].neighbor_count == 2 && "Wierzcholek 2 powinien miec 2 sasiadow");
    assert(graph.nodes[3].neighbor_count == 1 && "Wierzcholek 3 powinien miec 1 sasiada");
    
    // policz krawedzie
    count_edges(&graph);
    assert(graph.edges == 3 && "Nieprawidlowa liczba krawedzi w grafie");
    
    printf("Test operacji na krawedziach: OK\n");
    free_graph(&graph);
}

// test parametrow partycji
void test_partition_parameters() {
    Graph graph;
    inicialize_graph(&graph, 100);
    
    // test ustawienia liczby czesci
    assing_parts(&graph, 4);
    assert(graph.parts == 4 && "Nieprawidlowa liczba czesci");
    
    // test limitow wielkosci partycji
    assign_min_max_count(&graph, 4, 0.1); // 10% tolerancji
    
    assert(graph.min_count >= 0 && "Minimalna wielkosc partycji nie moze byc ujemna");
    assert(graph.max_count > graph.min_count && "Maksymalna wielkosc musi byc wieksza od minimalnej");
    
    printf("Test parametrow partycji: OK\n");
    free_graph(&graph);
}

int main() {
    printf("=== Testy Graph ===\n\n");
    
    test_graph_initialization();
    test_edge_operations();
    test_partition_parameters();
    
    printf("\n=== Wszystkie testy grafu zakonczone ===\n");
    return 0;
}