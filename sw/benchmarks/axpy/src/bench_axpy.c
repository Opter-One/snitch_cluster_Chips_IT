// Luca Colombo 2025 Chips-IT

#include "snrt.h"
#include "data.h"

// AXPY DOUBLE kernel: each core has its chunk to compute
void axpy_double(uint32_t start, uint32_t end, double a, double volatile *x,
                 double volatile *y, double volatile *z) {

    for (uint32_t i = start; i < end; i++) {
        z[i] = a * x[i] + y[i];
    }
    // Synchronize the FPUs
    snrt_fpu_fence();
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
    uint32_t end   = (core_idx == ncores - 1) ? L : start + chunk;

    // Compute cores work
    if (snrt_is_compute_core()) {

        //Warm up the cache
        for(volatile int i =0; i<2; i++){
            axpy_double(start, end, a, x, y, z);
        }
        
        if(core_idx==0){
            printf("Cache is warm!\n");
        }

        // Start counting the cycles and call the kernel
        uint64_t start_cycle = snrt_mcycle();
        axpy_double(start, end, a, x, y, z);
        uint64_t end_cycle = snrt_mcycle();

        // Performance
        cyclesc[core_idx] = (end_cycle - start_cycle);
        cycles_elementc[core_idx] = (double) cyclesc[core_idx] / (double) chunk;
        flop_cyclec[core_idx] = 2.0 * (double) chunk / (double) cyclesc[core_idx];
    }

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

        printf("AXPY performance\n");
        printf("Mean cycles %.0f \n", cycles_mean);
        printf("Mean cycles/element %.02f \n", cycles_elemean);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcpercluster);
        printf("z[0]=%.2f z[L-1]=%.2f\n", z[0], z[L-1]);
    }

    return 0;
}
