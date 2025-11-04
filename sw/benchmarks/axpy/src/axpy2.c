// Luca Colombo 2025 Chips-IT
#include "snrt.h"
#include "data.h"
#include "axpy_opt.h"

// Global cycles 
uint64_t start_cycle[16],end_cycle[16], total_cycles[16];

// Global chunks

uint32_t core_chunk[16];

// Pointers for each core to TCDM
float *local_a, *local_b, *local_axpy;

//Flag to use simple or optimized kernel
bool use_optimized = 0;

// Simple kernel with no use of FREP and SSRs
void axpy_simple(uint32_t chunk, float *a, float *b, float *axpy, float c){

    for(uint32_t i= 0; i<chunk; i++){
        axpy[i] = c* a[i] + b [i];
    }
    snrt_fpu_fence();
    return;
}


int main(){
    
    // Various metrics that will be used
    uint32_t core_idx = snrt_cluster_core_idx(); // ID number of each core
    uint32_t ncores   = snrt_cluster_compute_core_num(); // Number of cores

    uint32_t base_chunk = length/ncores; // Chunk of vectors for each core (base)
    core_chunk[core_idx] = base_chunk; // chunk of core that could change if length is not 
    // a multiple of ncores

    // If vector length is not a multiple of ncores
    uint32_t remainder_chunk = length % ncores;
    // Core n-1 does all the extra work (not optimal but better than nothing)
    if(remainder_chunk>0 && core_idx == ncores-1){
        core_chunk[core_idx] += remainder_chunk;
    }

    uint32_t offset_core = core_idx * base_chunk;


    snrt_cluster_hw_barrier(); // Barrier syncronization

    // The DM core initializes the vectors and TCDM (only DM core can call DMA)
    if(snrt_is_dm_core()){
        // Initialize the values
        for(uint32_t i = 0; i<length; i++){
            a[i] = (float)i;
            b[i] = (float)i;
            axpy[i] = 0.0;
        }
        // Initialize the pointers for each core (also dm, could be removed?)
        local_a = (float *)snrt_l1_next();
        local_b = local_a + length;
        local_axpy = local_b + length;
        // Allocate the vectors in memory, for each chunk (core)
        for(uint32_t cid = 0; cid<ncores; cid++){
            uint32_t offset = cid * base_chunk;

            snrt_dma_start_1d(local_a + offset, a + offset, core_chunk[cid]*sizeof(float));
            snrt_dma_start_1d(local_b + offset, b + offset, core_chunk[cid]*sizeof(float));
            snrt_dma_start_1d(local_axpy + offset, axpy + offset, core_chunk[cid]*sizeof(float));
            
            snrt_dma_wait_all();
        }
    }

    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Compute cores do the work and call the kernel, whilst counting cycles
    if(snrt_is_compute_core()){
        
        // Use either the simple non-optimized version declared in this file or
        // the optimized one in the vect_opt.h file
        // We add to local_* the offset of each core, so that in the kernel all the
        // operations are performed on the chunk of each core
        if(use_optimized){
            start_cycle[core_idx] = snrt_mcycle();
            axpy_opt(core_chunk[core_idx], local_a + offset_core, local_b + offset_core,
                 local_axpy + offset_core,c);
            end_cycle[core_idx] = snrt_mcycle();
        }
        else{
            start_cycle[core_idx] = snrt_mcycle();
            axpy_simple(core_chunk[core_idx], local_a + offset_core, local_b + offset_core,
                 local_axpy + offset_core,c);
            end_cycle[core_idx] = snrt_mcycle();
        }
    }

    snrt_cluster_hw_barrier(); // Barrier syncronization

    if(snrt_is_dm_core()){
        // Copy back from TCDM for each core result
        snrt_dma_start_1d(axpy, local_axpy, length*sizeof(float));
        snrt_dma_wait_all();
    }
    
    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Performance metrics from the core 0 (could use the DM core?)
    if(core_idx == 0){        

        uint64_t avg_cycles = 0;
        float avg_flop_cycle = 0.0;
        for(uint32_t i = 0; i<ncores; i++){
            total_cycles[i] = end_cycle[i] - start_cycle[i];
            avg_cycles += total_cycles[i];
            avg_flop_cycle += 2 * core_chunk[i] / (float) total_cycles[i]; // 1 floating op per element
        }
        avg_cycles /= ncores;
        avg_flop_cycle /= ncores;
        float avg_flop_cycle_general = (float)length / (float)avg_cycles;

        printf("AXPY %d add performance \n", length);
        printf("Avg. cycle %d and avg. flop/cycle per core %f and general %f \n",
             avg_cycles, avg_flop_cycle,avg_flop_cycle_general);
        
        if(length < 17){
            for(uint32_t i = 0; i < length; i++){
                printf("Result %d: %f\n", i, axpy[i]);
            }
        }
    }

    return 0;
}

