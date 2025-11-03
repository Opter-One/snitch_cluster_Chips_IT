#ifndef DATA_H
#define DATA_H

#ifndef length
#define length 16
#endif

static const double c = 1.5;
static  double a[length];
static  double b[length];
static  double axpy[length];

// Allocate shared arrays for stats
static uint64_t cyclesc[16];            // adjust if you have >16 cores
static double cycles_elementc[16];
static double flop_cyclec[16];

#endif