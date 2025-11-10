#ifndef DATA_H
#define DATA_H
// TCDM pointers to our data
double *x,*y,*z;
double a = 1.6789;

// Vector length (for now only a multiple of ncores, default 8)
#ifndef LEN
#define LEN 4120 // Really good performance with 4120-> flop/cycle 1.835236
#endif
 // If you exceed TCDM size you get generate errors
// Max size is 128KB/8bytes (64bits for doubles) = 16k theoretically, but even for 8k you get errors
// Need to check what is happening!

// Cycle and performance metrics
uint64_t start_cycle[16], end_cycle[16], total_cycles[16]; // Increase the num if ncores>16
double flop_cycle[16];

#endif