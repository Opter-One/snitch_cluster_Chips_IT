// Luca Colombo 2025 Chips-IT
#include "snrt.h"
#include "data.h"
#include "vect_opt.h"

// Global chunks

uint32_t core_chunk[16];

// Pointers for each core to TCDM
float *local_a, *local_b, *local_sum;

//Flag to use simple or optimized kernel
bool use_optimized = 1;
//Pre load TCDM !LEAVE ON! only for baseline set to 0
bool use_TCDM_preloading = 1;

// Simple kernel with no use of FREP and SSRs
void vect_add_simple(uint32_t chunk, float *a, float *b, float *sum,
    uint64_t* start_cycle,uint64_t *end_cycle){

    *start_cycle = snrt_mcycle();
    for(uint32_t i= 0; i<chunk; i++){
        sum[i] = a[i] + b [i];
    }
    snrt_fpu_fence();
    *end_cycle = snrt_mcycle();

    return;
}

// Simple kernel with no use of FREP and SSRs and no TCDDM
void vect_really_stupid(uint32_t start, uint32_t chunk, float *a, float *b, float *sum,
    uint64_t* start_cycle,uint64_t *end_cycle){
    
    *start_cycle = snrt_mcycle();
    for(uint32_t i= start; i<start+chunk; i++){
        sum[i] = a[i] + b [i];
    }
    snrt_fpu_fence();
    *end_cycle = snrt_mcycle();

    return;
}

int main(){
    
    // Various metrics that will be used
    uint32_t core_idx = snrt_cluster_core_idx(); // ID number of each core
    uint32_t ncores   = snrt_cluster_compute_core_num(); // Number of cores

    uint32_t base_chunk = L_vec/ncores; // Chunk of vectors for each core (base)
    uint32_t offset_core = core_idx * base_chunk;

    snrt_cluster_hw_barrier(); // Barrier syncronization

    // The DM core initializes the vectors and TCDM (only DM core can call DMA)
    if(snrt_is_dm_core()){
        // Initialize the values
        for(uint32_t i = 0; i<L_vec; i++){
            a[i] = (float)i+1;
            b[i] = (float)i+1;
            sum[i] = 0.0;
        }
        if(use_TCDM_preloading){
        // Initialize the pointers for each core (also dm, could be removed?)
            local_a = (float *)snrt_l1_next();
            local_b = local_a + L_vec;
            local_sum = local_b + L_vec;
            // Allocate the vectors in memory, for each chunk (core)
            for(uint32_t cid = 0; cid<ncores; cid++){
                uint32_t offset = cid * base_chunk;

                snrt_dma_start_1d(local_a + offset, a + offset, base_chunk*sizeof(float));
                snrt_dma_start_1d(local_b + offset, b + offset, base_chunk*sizeof(float));
                snrt_dma_start_1d(local_sum + offset, sum + offset, base_chunk*sizeof(float));
                
                
            }
            snrt_dma_wait_all();
            printf("Cache is warm!\n");
        }

    }


    snrt_cluster_hw_barrier(); // Barrier syncronization
 
    // Cycle counters
    uint64_t start_cycle,end_cycle;

    // Compute cores do the work and call the kernel, whilst counting cycles
    if(snrt_is_compute_core()){

        // Use either the simple non-optimized version declared in this file or
        // the optimized one in the vect_opt.h file
        // We add to local_* the offset of each core, so that in the kernel all the
        // operations are performed on the chunk of each core
        if(use_TCDM_preloading){
            if(use_optimized){
                // OPTIMIZED WITH FREP;SSR;TCDM!
                vect_add_opt(base_chunk, local_a + offset_core, local_b + offset_core,
                    local_sum + offset_core, &start_cycle, &end_cycle);
            }
            else{
                // OPTIMIZED WITH TCDM!
                vect_add_simple(base_chunk, local_a + offset_core, local_b + offset_core,
                    local_sum + offset_core, &start_cycle, &end_cycle);
            }
        }else{
            // coff, not optimized
            uint32_t start = core_idx * base_chunk;
            start_cycle = snrt_mcycle();
            vect_really_stupid(start, base_chunk, a, b, sum, &start_cycle, &end_cycle);
            end_cycle = snrt_mcycle();
        }

        // Performance
        cyclesc[core_idx] = (end_cycle - start_cycle);
        cycles_elementc[core_idx] = (float) cyclesc[core_idx] / (float) base_chunk;
        flop_cyclec[core_idx] =  (float) base_chunk / (float) cyclesc[core_idx];
    }

    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Copy back from TCDM for each core result only if we do not use SSRs
    if(snrt_is_dm_core() && use_TCDM_preloading){
        snrt_dma_start_1d(sum, local_sum, L_vec*sizeof(float));
        snrt_dma_wait_all();
    }
    
    snrt_cluster_hw_barrier(); // Barrier syncronization

    // Performance metrics from the core 0 (could use the DM core?)
    if(core_idx == 0){        

        double cycles_mean = 0;
        double cycles_elemean = 0;
        double flopcpercluster = 0;

        for (int i = 0; i < ncores; i++) {
            cycles_mean += cyclesc[i];
            cycles_elemean += cycles_elementc[i];
            flopcpercluster += flop_cyclec[i];
        }

        cycles_mean /= ncores;
        cycles_elemean /= ncores;
        double flopc = flopcpercluster / ncores;

        printf("Vector add %d performance\n", L_vec);
        if(use_TCDM_preloading){printf("Using TCDM preloading!\n");}
        if(use_optimized){printf("Using SSRs and FREP!\n");}
        printf("Mean cycles %.0f \n", cycles_mean);
        printf("Mean cycles/element %.02f \n", cycles_elemean);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcpercluster);
        
        if(L_vec < 17){
            for(uint32_t i = 0; i < L_vec; i++){
                printf("Result %d: %f\n", i, sum[i]);
            }
        }
    }

    return 0;
}

