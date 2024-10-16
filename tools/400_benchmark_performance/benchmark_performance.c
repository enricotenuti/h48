#include "../tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SOL_BUFFER_LEN 1000
#define MAX_SCRAMBLES 100  // Limite massimo per il numero di scrambles che può essere letto

char *scrambles[MAX_SCRAMBLES + 1];
char *solver;
int64_t size = 0;
char *buf;

struct ScrambleArgs {
    int scramble_idx;
};

// Funzione di timer per un singolo scramble
static double timerun_single_scramble(void (*run)(struct ScrambleArgs *, char *, int *), struct ScrambleArgs *args, char *solution, int *solution_length) {
    struct timespec start, end;
    double tdiff, tdsec, tdnano;

    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    run(args, solution, solution_length);  // Esegue la funzione run per il singolo scramble
    clock_gettime(CLOCK_MONOTONIC, &end);

    tdsec = end.tv_sec - start.tv_sec;
    tdnano = end.tv_nsec - start.tv_nsec;
    tdiff = tdsec + 1e-9 * tdnano;

    return tdiff;
}

// Modifica della funzione run_scramble per evitare ricorsione
void run_scramble(struct ScrambleArgs *args, char *solution, int *solution_length) {
    int scramble_idx = args->scramble_idx;  // Estrai l'indice dello scramble
    int64_t n;
    char sol[SOL_BUFFER_LEN], cube[22];

    printf("%d. %s\n", scramble_idx + 1, scrambles[scramble_idx]);
    printf("Solving scramble %s\n", scrambles[scramble_idx]);

    if (nissy_applymoves(NISSY_SOLVED_CUBE, scrambles[scramble_idx], cube) == -1) {
        printf("Invalid scramble\n");
        strcpy(solution, "Invalid scramble");
        *solution_length = 0;
        return;
    }

    n = nissy_solve(cube, solver, NISSY_NISSFLAG_NORMAL, 0, 20, 1, -1, size, buf, SOL_BUFFER_LEN, sol);
    if (n == 0) {
        printf("No solution found\n");
        strcpy(solution, "No solution found");
        *solution_length = 0;
    } else {
        printf("Solutions:\n%s\n", sol);
        strcpy(solution, sol);
        *solution_length = count_moves(solution, strlen(solution));
    }
}

void run_all_scrambles(void) {
    double times[MAX_SCRAMBLES];
    double total_time = 0.0, average_time = 0.0;

    printf("Solved the following scrambles with timings:\n\n");

    FILE *output_file = fopen("./tools/400_benchmark_performance/compressed_results.txt", "w");
    if (!output_file) {
        printf("Error: Could not open output file for writing\n");
        return;
    }

    int i;
    for (i = 0; scrambles[i] != NULL; i++) {
        struct ScrambleArgs args;
        args.scramble_idx = i;

        char solution[SOL_BUFFER_LEN];
        int solution_length = 0;

        double scramble_time = timerun_single_scramble(run_scramble, &args, solution, &solution_length);
        times[i] = scramble_time;
        total_time += scramble_time;

        fprintf(output_file, "%d. %s\n", i + 1, scrambles[i]);
        fprintf(output_file, "Solution: %s\n", solution);
        fprintf(output_file, "Length of solution: %d\n", solution_length);
        fprintf(output_file, "Time for scramble %d: %.4fs\n", i + 1, scramble_time);
        fprintf(output_file, "-------------------------\n");

        printf("Time for scramble %d: %.4fs\n", i + 1, scramble_time);
        printf("Lenght: %d\n", solution_length);
    }

    if (i > 0) {
        average_time = total_time / i;
    }

    printf("---------\n");
    printf("Total time for all scrambles: %.4fs\n", total_time);
    printf("Average time per scramble: %.4fs\n", average_time);

    fprintf(output_file, "---------\n");
    fprintf(output_file, "Total time for all scrambles: %.4fs\n", total_time);
    fprintf(output_file, "Average time per scramble: %.4fs\n", average_time);

    fclose(output_file);
}

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

int main(int argc, char **argv) {
    char tables_filename[255];
    char scrambles_filename[255] = "./tools/400_benchmark_performance/scrambles.txt";

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
