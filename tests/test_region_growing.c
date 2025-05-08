#include <stdio.h>
#include <assert.h>
#include "region_growing.h"
#include "graph.h"
#include "partition.h"
#include "file_reader.h"

// test dla malego grafu o znanej strukturze
void test_small_region_growing() {
    Graph graph;
    Partition_data partition_data;
    
    // stworz maly graf testowy (5 wierzcholkow w cyklu)
    inicialize_graph(&graph, 5);
    
    // dodaj krawedzie 0-1-2-3-4-0
    add_neighbor(&graph.nodes[0], 1);
    add_neighbor(&graph.nodes[1], 0);
    add_neighbor(&graph.nodes[1], 2);
    add_neighbor(&graph.nodes[2], 1);
    add_neighbor(&graph.nodes[2], 3);
    add_neighbor(&graph.nodes[3], 2);
    add_neighbor(&graph.nodes[3], 4);
    add_neighbor(&graph.nodes[4], 3);
    add_neighbor(&graph.nodes[4], 0);
    add_neighbor(&graph.nodes[0], 4);
    
    // inicjalizuj strukture partycji
    initialize_partition_data(&partition_data, 2);
    
    // podziel na 2 czesci z 20% tolerancja
    int success = region_growing(&graph, 2, &partition_data, 0.2);
    
    printf("Test malego grafu - podział na 2 czesci:\n");
    printf("- Sukces podzialu: %s\n", success ? "TAK" : "NIE");
    printf("- Rozmiary partycji: [%d, %d]\n", 
           partition_data.parts[0].part_vertex_count,
           partition_data.parts[1].part_vertex_count);
    
    // sprawdz spojnosc partycji
    check_partition_connectivity(&graph, 2);
    
    // zwolnij pamiec
    free_graph(&graph);
    free_partition_data(&partition_data, 2);
}

// test dla grafu z pliku
void test_file_region_growing() {
    Graph graph;
    ParsedData data = {0};
    Partition_data partition_data;
    
    // wczytaj graf z pliku
    load_graph("data/graf.csrrg", &graph, &data);
    initialize_partition_data(&partition_data, 3);
    
    printf("\nTest grafu z pliku - podział na 3 czesci:\n");
    
    // podziel na 3 czesci z 15% tolerancja
    int success = region_growing(&graph, 3, &partition_data, 0.15);
    
    printf("- Sukces podzialu: %s\n", success ? "TAK" : "NIE");
    printf("- Rozmiary partycji: [%d, %d, %d]\n",
           partition_data.parts[0].part_vertex_count,
           partition_data.parts[1].part_vertex_count,
           partition_data.parts[2].part_vertex_count);
    
    // sprawdz spojnosc partycji
    check_partition_connectivity(&graph, 3);
    
    // zwolnij pamiec
    free_graph(&graph);
    free_partition_data(&partition_data, 3);
}

int main() {
    printf("=== Testy Region Growing ===\n\n");
    
    test_small_region_growing();
    test_file_region_growing();
    
    printf("\n=== Koniec testow region growing===\n");
    return 0;
}