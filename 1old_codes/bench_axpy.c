// Luca Colombo 2025 Chips-IT

#include "snrt.h"
#include "data.h"


void axpy_double_opt(uint32_t start, uint32_t chunk, double a,
               double *x, double *y, double *z,
               uint64_t *start_cycle, uint64_t *end_cycle){

    int core_idx = snrt_cluster_core_idx();
    snrt_ssr_loop_1d(SNRT_SSR_DM_ALL, chunk, sizeof(double));

    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, x); //ft0
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_1D, y); //ft1
    snrt_ssr_write(SNRT_SSR_DM2, SNRT_SSR_1D, z); //ft2
 
    snrt_ssr_enable();

    asm volatile(
        "frep.o %[n_frep], 1, 0, 0 \n"
        "fmadd.d ft2, %[a], ft0, ft1\n"
        :
        : [ n_frep ] "r"(chunk - 1), [ a ] "f"(a)
        : "ft0", "ft1", "ft2", "memory");

    snrt_fpu_fence();
    snrt_ssr_disable();

    return;
}

// AXPY DOUBLE kernel: each core has its chunk to compute
void axpy_double(uint32_t start, uint32_t chunk, double a,
               double *x, double *y, double *z,
               uint64_t *start_cycle, uint64_t *end_cycle) {
    
    if (snrt_is_compute_core()) {

        *start_cycle = snrt_mcycle();
        for (uint32_t i = 0; i < chunk; i++) {
            z[i] = a * x[i] + y[i];
        }
        *end_cycle = snrt_mcycle();
        
        // Synchronize the FPUs
        snrt_fpu_fence();
    }
  
    snrt_cluster_hw_barrier(); // Sincronizza prima di scrivere

}

int main() {

    // check which core and how many
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t ncores   = snrt_cluster_compute_core_num();

    // Core 0 fills the vectors
    if (core_idx == 0) {
        // printf("Cluster has %u compute cores\n", snrt_cluster_compute_core_num());
        for (int i = 0; i < L; i++) {
            x[i] = (double)i;
            y[i] = (double)(L - i);
            z[i] = 0.0;
        }
    }
    uint32_t chunk = L / ncores;
    //Allocate space in TCDM
    double *local_x =
        (double *)snrt_l1_alloc_cluster_local(chunk, sizeof(double));
    double *local_y =
        (double *)snrt_l1_alloc_cluster_local(chunk, sizeof(double));
    double *local_z =
        (double *)snrt_l1_alloc_cluster_local(chunk, sizeof(double));

    if (snrt_is_dm_core()) {
        
        for (uint32_t cid = 0; cid < ncores; cid++) {
            
            uint32_t start = cid * chunk;

            snrt_dma_start_1d(local_x, x + start, chunk);
            snrt_dma_start_1d(local_y, y + start, chunk);
            snrt_dma_wait_all();
        }
    }

    // Synchronize the cores (wait for core 0 to end init)
    snrt_cluster_hw_barrier();

    // From where to where?
    uint32_t start = core_idx * chunk;

    uint64_t start_cycle,end_cycle;
    axpy_double(start, chunk, a, local_x, local_y, local_z, &start_cycle, &end_cycle);
        
    // Performance
    cyclesc[core_idx] = (end_cycle - start_cycle);
    cycles_elementc[core_idx] = (double) cyclesc[core_idx] / (double) chunk;
    flop_cyclec[core_idx] = 2.0 * (double) chunk / (double) cyclesc[core_idx];

    snrt_cluster_hw_barrier();

    // Simple confirmation (core 0 prints)
    if (core_idx == 0) {
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

        printf("AXPY %d performance\n", L);
        printf("Mean cycles %.0f \n", cycles_mean);
        printf("Mean cycles/element %.02f \n", cycles_elemean);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcpercluster);
        for(uint32_t i=0; i<L; i++){
            printf("z[%d]=%.2f \n",i, z[i]);
        }
        
    }

    return 0;
}
