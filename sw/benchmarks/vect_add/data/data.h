#ifndef DATA_H
#define DATA_H

#ifndef L_vec
#define L_vec 512
#endif

float a[L_vec],b[L_vec],sum[L_vec];

//Performance 
uint64_t cyclesc[16];
float cycles_elementc[16],flop_cyclec[16];
#endif