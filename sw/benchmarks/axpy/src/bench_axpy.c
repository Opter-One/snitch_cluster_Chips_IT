// Luca Colombo 2025 Chips-IT

#include "snrt.h"
#include "data.h"

// AXPY DOUBLE kernel: each core has its chunk to compute
void axpy_double(uint32_t start, uint32_t chunk, double a,
               double *x, double *y, double *z,
               uint64_t *start_cycle, uint64_t *end_cycle) {

    uint32_t core_idx = snrt_cluster_core_idx();
    
    snrt_cluster_hw_barrier(); // Sincronizza i core
    
    // Each core allocates its share of TCDM decided by the size of chunk
    double *local_x = (double *)snrt_l1_next();
    double *local_y = local_x + chunk;
    double *local_z = local_y + chunk;

    
    // DM core loads the vectors from the start of each core
    if (snrt_is_dm_core()) {
        snrt_dma_start_1d(local_x, x+start, chunk * sizeof(double));
        snrt_dma_start_1d(local_y, y+start, chunk * sizeof(double));
        snrt_dma_wait_all(); 
    }
    
    snrt_cluster_hw_barrier(); // Sincronizza i core

    if (snrt_is_compute_core()) {

        *start_cycle = snrt_mcycle();
        for (uint32_t i = 0; i < chunk; i++) {
            local_z[i] = a * local_x[i] + local_y[i];
            //printf("Core %d %f\n",snrt_cluster_core_idx(), local_x[i]);
        }
        *end_cycle = snrt_mcycle();
        
        // Synchronize the FPUs
        snrt_fpu_fence();
    }
  
    snrt_cluster_hw_barrier(); // Sincronizza prima di scrivere

    // Ogni core copia il proprio risultato
    if (snrt_is_compute_core()) {
        for (uint32_t i = 0; i < chunk; i++) {
            z[i] = local_z[i];
        }
    }
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

    // Synchronize the cores (wait for core 0 to end init)
    snrt_cluster_hw_barrier();

    // What is the size of the chunks per core?
    uint32_t chunk = L / ncores;
    // From where to where?
    uint32_t start = core_idx * chunk;

    uint64_t start_cycle,end_cycle;
    axpy_double(start, chunk, a, x, y, z, &start_cycle, &end_cycle);
        
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
