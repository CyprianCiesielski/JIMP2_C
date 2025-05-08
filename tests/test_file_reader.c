#include <stdio.h>
#include <assert.h>
#include "file_reader.h"
#include "graph.h"
#include "partition.h"

// pomocnicza funkcja do wyswietlania wyniku testu
void print_test_result(const char *test_name, int result) {
    printf("%s: %s\n", test_name, result ? "OK" : "FAIL");
}

// test wczytywania grafu
void test_graph_operations() {
    Graph graph;
    ParsedData data = {0};
    
    // wczytaj przykladowy graf - dostepny graf ma 18 wierzcholkow
    load_graph("data/graf.csrrg", &graph, &data);
    
    // sprawdz podstawowe wartosci
    assert(*(data.line1) == 18 && "Nieprawidlowa liczba wierzcholkow");
    assert(graph.nodes != NULL && "Wezel nie zostal zaalokowany");
    assert(graph.vertices == 105 && "Nieprawidlowa liczba wierzcholkow w grafie");
    
    // sprawdz czy wierzcholki maja poprawnie zainicjowane tablice sasiadow
    assert(graph.nodes[0].neighbors != NULL && "Sasiedzi nie zostali zalokowani");
    assert(graph.nodes[0].neighbor_count > 0 && "Brak sasiadow dla pierwszego wierzcholka");
    
    print_test_result("Test wczytywania grafu", 1);
    free_graph(&graph);
}

// test dodawania sasiada
void test_add_neighbor() {
    Node node = {0};
    node.neighbors = malloc(5 * sizeof(int));
    node.neighbor_capacity = 5;
    node.neighbor_count = 0;
    
    // dodaj sasiadow
    add_neighbor(&node, 1);
    add_neighbor(&node, 2);
    add_neighbor(&node, 3);
    
    // sprawdz czy zostali dodani poprawnie
    assert(node.neighbor_count == 3 && "Nieprawidlowa liczba sasiadow");
    assert(node.neighbors[0] == 1 && "Nieprawidlowy pierwszy sasiad");
    assert(node.neighbors[1] == 2 && "Nieprawidlowy drugi sasiad");
    assert(node.neighbors[2] == 3 && "Nieprawidlowy trzeci sasiad");
    
    print_test_result("Test dodawania sasiadow", 1);
    free(node.neighbors);
}

// test partycji
void test_partition() {
    Partition_data partition_data;
    initialize_partition_data(&partition_data, 2);
    
    // dodaj wierzcholki do pierwszej partycji
    add_partition_data(&partition_data, 0, 1);
    add_partition_data(&partition_data, 0, 2);
    add_partition_data(&partition_data, 0, 3);
    
    // dodaj wierzcholki do drugiej partycji
    add_partition_data(&partition_data, 1, 4);
    add_partition_data(&partition_data, 1, 5);
    
    // sprawdz czy wierzcholki sa w odpowiednich partycjach
    assert(partition_data.parts[0].part_vertex_count == 3 && "Zla liczba wierzcholkow w partycji 0");
    assert(partition_data.parts[1].part_vertex_count == 2 && "Zla liczba wierzcholkow w partycji 1");
    
    print_test_result("Test partycji", 1);
    free_partition_data(&partition_data, 2);
}

void run_file_reader_tests() {
    printf("Rozpoczynam testy czytania pliku...\n\n");
    
    test_graph_operations();
    test_add_neighbor();
    test_partition();
    
    printf("\nWszystkie testy czytania pliku zakonczone.\n");
}

int main() {
    printf("=== Testy File Reader ===\n\n");
    
    test_graph_operations();
    test_add_neighbor();
    test_partition();
    
    printf("\n=== Koniec testow file reader===\n");
    return 0;
}