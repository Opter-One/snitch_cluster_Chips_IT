// Luca Colombo 2025 Chips-IT

#include "snrt.h"
#include "data.h"
const bool ssr_en = 1; //Use SSR or not
const bool TCDM_en = 0; //Use TCDM or not

void dot_product(uint32_t start, uint32_t end,volatile double *acc, volatile double *A, 
    volatile double *B){

    if(ssr_en){
        register volatile double a asm("ft0");
        register volatile double b asm("ft1");  
        
        for (uint32_t i = start; i < end; i++) {
            *acc += a * b; // i valori di a e b arrivano automaticamente dagli SSR
        }
    }
    else{
        for(uint32_t i = start; i<end; i++){
            *acc += A[i]*B[i];
        }
    }
    
    snrt_fpu_fence();
    
    return;
}

//Setup of SSR 
void setup_ssrs_dotp(uint32_t chunk, double *local_A, double *local_B){
    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    asm volatile("" : "=f"(ft0), "=f"(ft1));

    snrt_ssr_loop_1d(SNRT_SSR_DM0, chunk, sizeof(double));
    snrt_ssr_loop_1d(SNRT_SSR_DM1, chunk, sizeof(double));
    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, local_A);
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_1D, local_B);

    register volatile double res_ssr asm("fs0") = 0;
    (void)res_ssr;
    return;
}



int main(){

    // check which core and how many
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t ncores   = snrt_cluster_compute_core_num();

    uint32_t chunk = L/ncores;
    uint32_t start = core_idx*chunk;
    uint32_t end = (core_idx == ncores - 1) ? L : start + chunk;

    static double sum_core[16];

    //Initialize the vectors
    if(core_idx == 0){
        for (uint32_t i=0; i<L; i++){
            A[i] = (double) i;
            B[i] = (double) (L-i);
        }
    }
    
    double *local_A, *local_B, *partial_sums;

    // Copy data in TCDM
    if (snrt_is_dm_core() && TCDM_en) {
        // Allocate space in TCDM
        local_A = (double *)snrt_l1_next();
        local_B = local_A + L;
        size_t size = L * sizeof(double);
        snrt_dma_start_1d(local_A, A, size);
        snrt_dma_start_1d(local_B, B, size);
        snrt_dma_wait_all();
    }

    // Synchronize the cores (wait for core 0 to end init)
    snrt_cluster_hw_barrier();


    if(snrt_is_compute_core()){

        uint64_t start_cycle, end_cycle;


        sum_core[core_idx] = 0.0;
        
        if(ssr_en){

            

            if(TCDM_en){
                setup_ssrs_dotp(chunk,local_A+start,local_B+start);
                snrt_ssr_enable();

                //Read the cycles
                start_cycle = snrt_mcycle();
                dot_product(0,chunk,&sum_core[core_idx],local_A+start,local_B+start);
                end_cycle = snrt_mcycle();

            }else{

                setup_ssrs_dotp(chunk,&A,&B);
                snrt_ssr_enable();

                for(uint32_t i=0; i<2; i++){
                    dot_product(start,end,&sum_core[core_idx],A,B);
                }
                if(core_idx==0){printf("Cache is warm!\n");}
                sum_core[core_idx] = 0.0;
                //Read the cycles
                start_cycle = snrt_mcycle();
                dot_product(start,end,&sum_core[core_idx],A,B);
                end_cycle = snrt_mcycle();
            }
            
        }else{

            if(TCDM_en){
                //Read the cycles
                start_cycle = snrt_mcycle();
                dot_product(0,chunk,&sum_core[core_idx],local_A+start,local_B+start);
                end_cycle = snrt_mcycle();
            }else{
                for(uint32_t i=0; i<2; i++){
                    dot_product(start,end,&sum_core[core_idx],A,B);
                }
                if(core_idx==0){printf("Cache is warm!\n");}
                sum_core[core_idx] = 0.0;
                //Read the cycles
                start_cycle = snrt_mcycle();
                dot_product(start,end,&sum_core[core_idx],A,B);
                end_cycle = snrt_mcycle();
            }
            
        }

        // Performance
        cyclesc[core_idx] = (end_cycle - start_cycle);
        cycles_elementc[core_idx] = (double) cyclesc[core_idx] / (double) chunk;
        flop_cyclec[core_idx] = (double) chunk / (double) cyclesc[core_idx];
    }

     snrt_cluster_hw_barrier();

    // Simple confirmation (core 0 prints)
    if (core_idx == 0) {

        //Result
        double sum = 0.0;
         for (uint32_t i = 0; i < ncores; i++){
            sum += sum_core[i];
         }
            
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

        

        printf("Dot product (%d vectors) performance\n", L);
        printf("Mean cycles %.0f \n", cycles_mean);
        printf("Mean cycles/element %.02f \n", cycles_elemean);
        printf("Mean FLOP/cycle %.5f \n", flopc);
        printf("FLOP/cycle per cluster %.5f \n", flopcpercluster);
        printf("Dot = %.5f\n",sum);
    }



    return 0;
} 