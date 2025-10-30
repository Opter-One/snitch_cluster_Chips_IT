#ifndef DATA_H
#define DATA_H

#ifndef L
#define L 64
#endif

#ifndef perf_num
#define perf_num 16
#endif

static volatile double X[L*L];
static volatile double Y[L*L];
static volatile double Z[L*L];

// Allocate shared arrays for stats
static uint64_t cyclesc[perf_num];            // adjust if you have >8 cores
static double cycles_elementc[perf_num];
static double flop_cyclec[perf_num];

#endif