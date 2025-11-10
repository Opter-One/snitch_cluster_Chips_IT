// Luca Colombo Chips-IT 2025
/* Vector DOT product that uses data allocated in TCDM by the DM core to feed SSR registers. 
   Implements the dot using FREP as the loop body. */

// Snitch runtime library
#include "snrt.h"
#include "data.h"
#include "vect_inter_dot_opt.h"

bool PRINT_INTER_RESULTS = 0; // Print intermidiate results of each core

int main(){
    // Core ID and core count
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t ncores = snrt_cluster_compute_core_num();

    // Compute the chunk of the vector per core, and the offset that is used to space them
    uint32_t chunk_per_core = LEN/ncores;

    // DM core allocates memory in TCDM and initializes the vectors
    if(snrt_is_dm_core()){

        // Pointers to TCDM memory, spaced by LEN 
        x = (double *)snrt_l1_next();
        y = x + LEN;
        intermidiate_dot = y + LEN;
        dot_core = intermidiate_dot + LEN; // DOT is only one element per core!
        dot_final = dot_core + ncores; // final result

        // If pointers are null -> break
        if (!x || !y || !dot_core || !intermidiate_dot || !dot_final) {
            printf("Memory allocation failed!\n");
            return -1;
        } 

        // Initialize the values of vectors, can change as you like
        for(uint32_t i = 0; i<LEN; i++){
            x[i] = (double)i*0.22;
            y[i] = (double)(LEN-i)*0.55;
        }
        *dot_final = 0.0;
    }

    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Only the compute cores do something
    if(snrt_is_compute_core()){

        if(chunk_per_core == 0){
            printf("Chunk for each core is 0!\n");
            return -2;
        }
        // Offset to index the correct chunk of data per core
        uint32_t offset = core_idx*chunk_per_core;
        
        // Call the kernel, dot_core + core_idx is to index the correct element for each core
        vect_inter_dot_opt(chunk_per_core, offset, &start_cycle[core_idx],&end_cycle[core_idx],
                    x, y, intermidiate_dot, dot_core + core_idx);

        total_cycles[core_idx] = end_cycle [core_idx] - start_cycle[core_idx];
        flop_cycle[core_idx] = (double) 2 *chunk_per_core/(double) total_cycles[core_idx];
        // Times two because we multiply and then add
    }

    snrt_cluster_hw_barrier(); // Barrier syncronization
    
    // Core 0 does last calcultations and prints metrics/results
    if(core_idx==0){

        // Accumulate the results of each core into the final value
        for(uint32_t cid=0; cid<ncores; cid++) {
            *dot_final += dot_core[cid];
        }

        // Mean performance values
        uint64_t mean_cycles=0;
        double mean_flop_cycle = 0.0;

        for(uint32_t cid = 0; cid < ncores; cid ++){
            mean_cycles += total_cycles[cid];
            mean_flop_cycle += flop_cycle[cid];
        }
        mean_cycles /= ncores;
        mean_flop_cycle /= ncores;

        printf("Vector DOT %d performance\n",LEN);
        printf("Mean cycles: %llu\n", (unsigned long long)mean_cycles);
        printf("Mean FLOP/cycle: %f\n", mean_flop_cycle);

        // Print results for sanity check
        if(PRINT_INTER_RESULTS){
            printf("Intermidiate results\n");
            for(uint32_t cid=0; cid<ncores; cid++) {
                printf("Core %d: %f \n", cid, dot_core[cid]);
            }
        }
        printf("DOT product: %f\n", *dot_final);
        
    }
    return 0;
}

/* NEVER USE FLOAT TYPE! BREAKS EVERYTHING! */