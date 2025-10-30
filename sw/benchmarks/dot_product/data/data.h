#ifndef DATA_H
#define DATA_H

#ifndef L
#define L 16
#endif

static volatile double A[L];
static volatile double B[L];

// Allocate shared arrays for stats
static uint64_t cyclesc[16];            // adjust if you have >16 cores
static double cycles_elementc[16];
static double flop_cyclec[16];

#endif