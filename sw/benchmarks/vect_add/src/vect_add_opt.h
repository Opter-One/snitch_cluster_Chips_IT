// Luca Colombo 2025 Chips-IT
// Optimized version with SSRs and FREP operation

#include "snrt.h"

void vect_add_opt(uint32_t chunk, float *a, float *b, float *sum, 
    uint64_t* start_cycle,uint64_t *end_cycle){
    // Start of SSR region.
    // Declare the registers
    register volatile float ft0 asm("ft0");
    register volatile float ft1 asm("ft1");
    register volatile float ft2 asm("ft2");

    // Declare the registers as volatile to avoid compiler optimization (i think)
    asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));
    
    // Setup the 1d loop with ssr (tell which strems to use, the size and the size of
    // the elements)
    snrt_ssr_loop_1d(SNRT_SSR_DM0, chunk, sizeof(float));
    snrt_ssr_loop_1d(SNRT_SSR_DM1, chunk, sizeof(float));
    snrt_ssr_loop_1d(SNRT_SSR_DM2, chunk, sizeof(float));

    // Read from ft0 and ft1 that will be wired to a and b 
    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, a); //ft0
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_1D, b); //ft1
    // Write to ft2 (sum)
    snrt_ssr_write(SNRT_SSR_DM2, SNRT_SSR_1D, sum);

    *start_cycle = snrt_mcycle();
    // Enable the SSRs
    snrt_ssr_enable();

    snrt_fpu_fence();

    // Assembly code to add the ft0 and ft1 to ft2
    asm volatile(
        "frep.o %[n_frep], 1, 0, 0 \n"
        "fadd.s ft2, ft0, ft1\n"
        :
        : [ n_frep ] "r"(chunk - 1)
        : "ft0", "ft1", "ft2", "memory");
    
    snrt_ssr_disable();
    
    snrt_fpu_fence();

    *end_cycle = snrt_mcycle();
    
    return;
}

/* The FREP instruction repeats n_frep+1 times the next set of instructions, depending on
 the value after n_frep: in this case it is 1, so 1 instruction. The instruction is 
 a simple floating point add. After the loop the FPUs are syncronized and SSRs are 
 disabled. */