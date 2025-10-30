//Lesson: no 2d matrices, they are put in memory row after row, better to use 1d indexes

#include "snrt.h"
#include "data.h"

#define BS 32  // block size, e.g 4, 8, 16...
double result_noblock[2];

void matrix_mult_blocked(uint32_t start, uint32_t end,
                         volatile double *X, volatile double *Y, volatile double *Z) {
    //These 3 for move between blocks with the size of BSxBS (sub matrices)
    for (int ii = start; ii < end; ii += BS) {
        for (int jj = 0; jj < L; jj += BS) {
            for (int kk = 0; kk < L; kk += BS) {
                // reduces memory conjestions by computing on smaller matrices
                for (int i = 0; i < BS && (ii + i) < end; i++) {
                    for (int j = 0; j < BS && (jj + j) < L; j++) {
                        double acc = Z[(ii+i)*L + (jj+j)];
                        for (int k = 0; k < BS && (kk + k) < L; k++) {
                            acc += X[(ii+i)*L + (kk+k)] * Y[(kk+k)*L + (jj+j)];
                        }
                        Z[(ii+i)*L + (jj+j)] = acc;
                    }
                }
            }
        }
    }
    snrt_fpu_fence();
}

//Calculate the matrix product
void matrix_mult(uint32_t start, uint32_t end, volatile double *X, volatile double *Y, 
    volatile double *Z){
    
    //We cycle between rows (for X) and columns (for Y)
    for(int i = start; i<end; i++){
        for(int j = 0; j<L; j++){
            double acc = 0.0;
            for(int k = 0; k<L; k++){
                acc += X[i*L +k]*Y[k*L +j];
            }
            Z[i*L +j] = acc;
        }
    }
    //Syncronize all FPUs
    snrt_fpu_fence();
    return;
}

int main(){

    // check which core and how many
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t ncores   = snrt_cluster_compute_core_num();

    //We need to calculate the chuncks for each core
    uint32_t chunk = L/ncores;
    uint32_t start = core_idx * chunk;
    uint32_t end   = (core_idx == ncores - 1) ? L : start + chunk;

    if(core_idx==0){
        //Inizialize the matrices
        for(int i = 0; i<L; i++){
            for(int j = 0; j<L; j++){
                X[i*L+j] = (double) i*j; //Start from row 0 and move with J until L, then move to row 1 (L distance from 0)
                Y[i*L+j] = (double) j;
            }
        }
    }

    //Syncronize the cores
    snrt_cluster_hw_barrier();

    if(snrt_is_compute_core()){
        //Warm up the cache (only for matrices under 32x32)
        if(L<33){
            matrix_mult(start,end,X,Y,Z);
            if(core_idx==0){
                printf("Cache is warm!\n");
            }
        }else{if(core_idx==0){printf("Size too big to warmup cache\n");}}

        // Start counting the cycles and call the kernel non blocked 
        uint64_t start_cycle = snrt_mcycle();
        //matrix_mult(start, end, X, Y, Z);
        uint64_t end_cycle = snrt_mcycle();

        // Performance
        cyclesc[core_idx] = (end_cycle - start_cycle);
        cycles_elementc[core_idx] = (double) cyclesc[core_idx] / (double) ((end-start)*L);
        flop_cyclec[core_idx] = (2.0 * (end - start) * L * L) / (double)cyclesc[core_idx];

        if(core_idx==0){
            //save some results to check they compute equally lol
            result_noblock[0]= Z[0];
            result_noblock[1]= Z[(L-1)*L + (L-1)];
            //Reset Z matrix
            for(int i = 0; i<L; i++){
                for(int j = 0; j<L; j++){
                    Z[i*L+j]=0.0;
                }
            }
            printf("No blocking complete\n");
        }
    }

    //Resyncronize the cores before new computation
    snrt_cluster_hw_barrier();

    if(snrt_is_compute_core()){
        // Start counting the cycles and call the kernel non blocked 
        uint64_t start_cycleb = snrt_mcycle();
        matrix_mult_blocked(start, end, X, Y, Z);
        uint64_t end_cycleb = snrt_mcycle();

        // Performance
        cyclesc[core_idx+ncores] = (end_cycleb - start_cycleb);
        cycles_elementc[core_idx+ncores] = (double) cyclesc[core_idx+ncores] / (double) ((end-start)*L);
        flop_cyclec[core_idx+ncores] = (2.0 * (end - start) * L * L) / (double)cyclesc[core_idx+ncores];
    }

    snrt_cluster_hw_barrier();

    // Performance calculations and prints (core 0 prints)
    if (core_idx == 0) {

        //Non blocked metrics
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

        printf("Matrix %dx%d performance\n",L,L,BS,BS);
        printf("Mean cycles %.0f \n", cycles_mean);
        printf("Mean cycles/element %.02f \n", cycles_elemean);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcpercluster);
        printf("Z[0,0]=%.2f Z[L-1,L-1]=%.2f\n", result_noblock[0], result_noblock[1]);

        //Blocked metrics
        double cycles_meanb = 0;
        double cycles_elemeanb = 0;
        double flopcperclusterb = 0;

        for (int i = ncores; i < perf_num; i++) {
            cycles_meanb += cyclesc[i];
            cycles_elemeanb += cycles_elementc[i];
            flopcperclusterb += flop_cyclec[i];
        }

        cycles_meanb /= ncores;
        cycles_elemeanb /= ncores;
        flopc = flopcperclusterb / ncores;

        printf("Matrix %dx%d with %dx%d blocks performance\n",L,L,BS,BS);
        printf("Mean cycles %.0f \n", cycles_meanb);
        printf("Mean cycles/element %.02f \n", cycles_elemeanb);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcperclusterb);
        printf("Z[0,0]=%.2f Z[L-1,L-1]=%.2f\n", Z[0], Z[(L-1)*L + (L-1)]);
    }


    return 0;
}