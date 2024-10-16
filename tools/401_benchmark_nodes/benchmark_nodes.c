#include "../tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SOL_BUFFER_LEN 1000
#define MAX_SCRAMBLES 100  // Limite massimo per il numero di scrambles che può essere letto

// Variabili globali
char *scrambles[MAX_SCRAMBLES + 1];
char *solver;
int64_t size = 0;
char *buf;

// Utilizzo di typedef per le strutture

// Argomenti dello scramble
typedef struct {
    int scramble_idx;
} ScrambleArgs;

// Informazioni della soluzione
typedef struct {
    char solution[SOL_BUFFER_LEN];
    int solution_length;
} SolutionInfo;

// Informazioni sullo scramble (tempo e nodi visitati)
typedef struct {
    double time_taken;
    unsigned long long nodes_visited;
} ScrambleInfo;

// Funzione di timer per un singolo scramble
static double timerun_single_scramble(void (*run)(ScrambleArgs *, SolutionInfo *, ScrambleInfo *), 
                                      ScrambleArgs *args, SolutionInfo *solution_info, 
                                      ScrambleInfo *scramble_info) {
    struct timespec start, end;
    double tdiff, tdsec, tdnano;

    fflush(stdout);
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Esegue la funzione run per il singolo scramble
    run(args, solution_info, scramble_info);

    clock_gettime(CLOCK_MONOTONIC, &end);

    tdsec = end.tv_sec - start.tv_sec;
    tdnano = end.tv_nsec - start.tv_nsec;
    tdiff = tdsec + 1e-9 * tdnano;

    return tdiff;
}

// Funzione per eseguire un singolo scramble
void run_scramble(ScrambleArgs *args, SolutionInfo *solution_info, ScrambleInfo *scramble_info) {
    int scramble_idx = args->scramble_idx;  // Estrai l'indice dello scramble
    int64_t n;
    char cube[22];

    printf("%d. %s\n", scramble_idx + 1, scrambles[scramble_idx]);
    printf("Solving scramble %s\n", scrambles[scramble_idx]);

    // Applicazione del scramble al cubo
    if (nissy_applymoves(NISSY_SOLVED_CUBE, scrambles[scramble_idx], cube) == -1) {
        printf("Invalid scramble\n");
        strcpy(solution_info->solution, "Invalid scramble");
        solution_info->solution_length = 0;
        return;
    }

    // Risoluzione dello scramble
    n = nissy_solve_nodes(cube, solver, NISSY_NISSFLAG_NORMAL, 0, 20, 1, -1, size, buf, SOL_BUFFER_LEN, 
                          solution_info->solution, &scramble_info->nodes_visited);

    if (n == 0) {
        printf("No solution found\n");
        strcpy(solution_info->solution, "No solution found");
        solution_info->solution_length = 0;
    } else {
        printf("Solutions:\n%s\n", solution_info->solution);
        solution_info->solution_length = count_moves(solution_info->solution, strlen(solution_info->solution));
    }
}

// Funzione per eseguire tutti gli scrambles
void run_all_scrambles(void) {
    double total_time = 0.0, average_time = 0.0;
    unsigned long long total_nodes = 0;
    double average_nodes = 0.0;

    printf("Solved the following scrambles with timings:\n\n");

    FILE *output_file = fopen("./tools/401_benchmark_nodes/compressed_results.txt", "w");
    if (!output_file) {
        printf("Error: Could not open output file for writing\n");
        return;
    }

    int i;
    for (i = 0; scrambles[i] != NULL; i++) {
        ScrambleArgs args;
        SolutionInfo solution_info;
        ScrambleInfo scramble_info;

        args.scramble_idx = i;
        solution_info.solution_length = 0;
        scramble_info.nodes_visited = 0;

        // Misura il tempo di esecuzione per ogni scramble
        scramble_info.time_taken = timerun_single_scramble(run_scramble, &args, &solution_info, &scramble_info);

        total_time += scramble_info.time_taken;
        total_nodes += scramble_info.nodes_visited;

        // Scrive i risultati sul file
        fprintf(output_file, "%d. %s\n", i + 1, scrambles[i]);
        fprintf(output_file, "Solution: %s\n", solution_info.solution);
        fprintf(output_file, "Length of solution: %d\n", solution_info.solution_length);
        fprintf(output_file, "Nodes visited: %llu\n", scramble_info.nodes_visited);
        fprintf(output_file, "Avg time per node: %.4fns\n", (scramble_info.time_taken / scramble_info.nodes_visited)*1e9);
        fprintf(output_file, "Time: %.4fs\n", scramble_info.time_taken);
        fprintf(output_file, "-------------------------\n");

        printf("Time for scramble %d: %.4fs\n", i + 1, scramble_info.time_taken);
        printf("Nodes visited for scramble %d: %llu\n", i + 1, scramble_info.nodes_visited);
    }

    if (i > 0) {
        average_time = total_time / i;
        average_nodes = (double)total_nodes / i;
    }

    printf("---------\n");
    printf("Total time for all scrambles: %.4fs\n", total_time);
    printf("Average time per scramble: %.4fs\n", average_time);
    printf("Average nodes visited per scramble: %.2f\n", average_nodes);

    fprintf(output_file, "---------\n");
    fprintf(output_file, "Total time for all scrambles: %.4fs\n", total_time);
    fprintf(output_file, "Average time per scramble: %.4fs\n", average_time);
    fprintf(output_file, "Average nodes visited per scramble: %.2f\n", average_nodes);
    fprintf(output_file, "Average time per node: %.4fs\n", total_time / total_nodes);


    fclose(output_file);
}

// Funzione per leggere gli scrambles da un file
int read_scrambles_from_file(const char *filename, char **scrambles) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "//", 2) == 0 || strlen(line) == 0)
            continue;

        scrambles[count] = (char *)malloc(strlen(line) + 1);
        if (scrambles[count] == NULL) {
            printf("Error: Memory allocation failed\n");
            fclose(file);
            return -1;
        }
        strcpy(scrambles[count], line);
        count++;

        if (count >= MAX_SCRAMBLES) {
            printf("Warning: Reached maximum number of scrambles (%d)\n", MAX_SCRAMBLES);
            break;
        }
    }

    scrambles[count] = NULL;
    fclose(file);
    return 0;
}

// Funzione main
int main(int argc, char **argv) {
    char tables_filename[255];
    char scrambles_filename[255] = "./tools/401_benchmark_nodes/scrambles.txt";

    if (argc < 2) {
        printf("Error: not enough arguments. A solver must be given.\n");
        return 1;
    }

    solver = argv[1];

    srand(time(NULL));
    nissy_setlogger(log_stderr);

    sprintf(tables_filename, "tables/%s", solver);
    if (getdata(solver, &buf, tables_filename) != 0)
        return 1;

    size = nissy_datasize(solver);

    if (read_scrambles_from_file(scrambles_filename, scrambles) != 0) {
        printf("Error: failed to read scrambles from file\n");
        free(buf);
        return 1;
    }

    run_all_scrambles();

    free(buf);

    for (int i = 0; scrambles[i] != NULL; i++) {
        free(scrambles[i]);
    }

    return 0;
}
