#ifndef FM_OPTIMIZATION_H
#define FM_OPTIMIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "graph.h"
#include "partition.h"

struct Graph;
typedef struct Graph Graph;
struct Partition_data;
typedef struct Partition_data Partition_data;

typedef struct
{
    int vertex;
    int gain;
    int target_part;
} Move;

typedef struct
{
    Move **moves;
    int max_gain;
    int min_gain;
} Bucket;

typedef struct
{
    Graph *graph;
    Partition_data *partition;
    bool *locked;
    int *gains;
    int *target_parts;
    int max_iterations;
    int *part_sizes;
    int iterations;
    int moves_made;
    int initial_cut;
    int current_cut;
    int best_cut;
    int *best_partition;
} FM_Context;

void cut_edges_optimization(Graph *graph, Partition_data *partition_data, int max_iterations);

FM_Context *initialize_fm_context(Graph *graph, Partition_data *partition_data, int max_iterations);
void free_fm_context(FM_Context *context);
void identify_boundary_vertices(FM_Context *context, bool *is_boundary);
int calculate_initial_cut(FM_Context *context);
int calculate_gain(FM_Context *context, int vertex, int target_part);
int is_valid_move(FM_Context *context, int vertex, int target_part);
void apply_move(FM_Context *context, int vertex, int target_part);
void update_gains_after_move(FM_Context *context, int moved_vertex);
int find_best_move(FM_Context *context, bool *is_boundary);
void print_cut_statistics(FM_Context *context);
void save_best_solution(FM_Context *context);
void restore_best_solution(FM_Context *context);

int will_remain_connected_if_removed(Graph *graph, int vertex);
int is_move_valid_with_integrity(FM_Context *context, int vertex, int target_part);
int apply_move_safely(FM_Context *context, int vertex, int target_part);

void check_partition_connectivity(Graph *graph, int parts);

#endif