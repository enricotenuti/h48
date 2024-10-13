/*
This header file exposes certain functions that are meant to be used
for testing purposes only.
*/

#define STATIC static
#define LOG printf

#include "../src/solvers/tables.h"

size_t gendata_h48_derive(uint8_t, const void *, void *);
int parse_h48_solver(const char *, uint8_t [static 1], uint8_t [static 1]);
long long int nissy_datainfo(
    uint64_t, const char *, void (*)(const char *, ...));
long long int nissy_derivedata(const char *, const void *, void *);
