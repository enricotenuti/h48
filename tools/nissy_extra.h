/*
This header file exposes certain functions that are meant to be used
for testing purposes only.
*/

#define STATIC static
#define LOG printf

#include "../src/solvers/tables.h"

size_t gendata_h48_derive(uint8_t, const void *, void *);
int parse_h48_options(const char *, uint8_t *, uint8_t *, uint8_t *);
