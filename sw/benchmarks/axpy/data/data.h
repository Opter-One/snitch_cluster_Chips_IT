#ifndef DATA_H
#define DATA_H

#ifndef L
#define L 16
#endif

static const double a = 1.5;
static  double x[L];
static  double y[L];
static  double z[L];

// Allocate shared arrays for stats
static uint64_t cyclesc[16];            // adjust if you have >16 cores
static double cycles_elementc[16];
static double flop_cyclec[16];

#endif