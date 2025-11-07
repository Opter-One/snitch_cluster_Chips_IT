// Luca Colombo Chips-IT 2025
/* Vector add that uses data allocated in TCDM by the DM core to feed SSR registers. 
   Implements the add using FREP as the loop body. */

// Snitch runtime library
#include "snrt.h"

// Print first 5 and last 5 results
bool PRINT_RESULTS = 1;

// Vector length (for now only a multiple of ncores, default 8)
uint32_t LEN = 4096; // If you exceed TCDM size you get generate errors
// Max size is 128KB/8bytes (64bits for doubles) = 16k theoretically, but even for 8k you get errors
// Need to check what is happening!

// TCDM pointers to our data
double *a,*b,*sum;

// Cycle and performance metrics
uint64_t start_cycle[8], end_cycle[8], total_cycles[8];
double flop_cycle[8];

int main(){
    // Core ID and core count
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t ncores = snrt_cluster_compute_core_num();

    // DM core allocates memory in TCDM and initializes the vectors
    if(snrt_is_dm_core()){

        // Pointers to TCDM memory, spaced by LEN 
        a = (double *)snrt_l1_next();
        b = a + LEN;
        sum = b + LEN;

        // If pointers are null break
        if (!a || !b || !sum) {
            printf("Memory allocation failed!\n");
            return -1;
        } 

        // Initialize the values of vectors
        for(uint32_t i = 0; i<LEN; i++){
            a[i] = (double)i+1;
            b[i] = (double)i+1;
            sum[i] = 0.0;
        }

    }

    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Only the compute cores do something
    if(snrt_is_compute_core()){

        // Compute the chunk of the vector per core, and the offset that is used to space them
        uint32_t chunk_per_core = LEN/ncores;
        uint32_t offset = core_idx*chunk_per_core;
        
        // Setup the 1d loop with ssr (tell which streams to use, the size and the size of
        // the elements)
        snrt_ssr_loop_1d(SNRT_SSR_DM0, chunk_per_core, sizeof(double));
        snrt_ssr_loop_1d(SNRT_SSR_DM1, chunk_per_core, sizeof(double));
        snrt_ssr_loop_1d(SNRT_SSR_DM2, chunk_per_core, sizeof(double));

        // Read from ft0 and ft1 that will be wired to a and b 
        snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, a+offset); //ft0->a
        snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_1D, b+offset); //ft1->b
        // Write to ft2 (sum)
        snrt_ssr_write(SNRT_SSR_DM2, SNRT_SSR_1D, sum+offset); //ft2->sum

        start_cycle[core_idx] = snrt_mcycle();
        // Enable the SSRs
        snrt_ssr_enable();
        
        // Assembly code to add ft0 and ft1 to ft2
        asm volatile(
            "frep.o %[n_frep], 1, 0, 0 \n"
            "fadd.d ft2, ft0, ft1\n"
            :
            : [ n_frep ] "r"(chunk_per_core - 1)
            : "ft0", "ft1", "ft2", "memory");

        // Disable SSRs
        snrt_ssr_disable();
        // Fence for FPU syncronization
        snrt_fpu_fence();

        end_cycle[core_idx] = snrt_mcycle();
        
        // Performance calculations for each core
        total_cycles[core_idx] = end_cycle[core_idx]-start_cycle[core_idx];
        flop_cycle[core_idx] = (double) chunk_per_core/ (double) total_cycles[core_idx];
    }


    snrt_cluster_hw_barrier(); // Barrier syncronization
            
    if(core_idx==0){

        // Mean performance values
        uint64_t mean_cycles=0;
        double mean_flop_cycle = 0.0;

        for(uint32_t cid = 0; cid < ncores; cid ++){
            mean_cycles += total_cycles[cid];
            mean_flop_cycle += flop_cycle[cid];
        }
        mean_cycles /= ncores;
        mean_flop_cycle /= ncores;

        printf("Vector add %d performance\n",LEN);
        printf("Mean cycles: %llu\n", (unsigned long long)mean_cycles);
        printf("Mean FLOP/cycle: %f\n", mean_flop_cycle);

        // Print results for sanity check
        if(PRINT_RESULTS){
            for(uint32_t i=0; i<5; i++){
                printf("Sum(%d): %f\n",i, sum[i]);
            }
            for(uint32_t i=LEN-5; i<LEN; i++){
                printf("Sum(%d): %f\n",i, sum[i]);
            }
        }
    }
    return 0;
}

/* NEVER USE FLOAT TYPE! BREAKS EVERYTHING! */