// Luca Colombo Chips-IT 2025
/* As written in the SSR paper you cannot read a register that is defined as write stream.
   So to implement the dot firstly snitch performs the multiplication per element and saves every result
   in an element of intermidiate_dot (wired to ft2). Then it disables the ssr, reconfigures ft2 
   to be a read stream and accumulates the results in ft3 (initialized to 0). It is not possible
   to use SSRs for ft3 for the previous problem.
*/
void vect_inter_dot_opt(uint32_t chunk_per_core, uint32_t offset, 
                        uint64_t *start_cycle, uint64_t *end_cycle,
                        double *x, double *y, double *intermidiate_dot, double *dot_core){
    
    // Setup the 1d loop with ssr (tell which streams to use, the size and the size of
    // the elements)
    snrt_ssr_loop_1d(SNRT_SSR_DM0, chunk_per_core, sizeof(double));
    snrt_ssr_loop_1d(SNRT_SSR_DM1, chunk_per_core, sizeof(double));
    snrt_ssr_loop_1d(SNRT_SSR_DM2, chunk_per_core, sizeof(double));

    // Read from ft0 and ft1 that will be wired to x and y
    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, x + offset); //ft0->x
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_1D, y + offset); //ft1->y
    // Write to ft2 the multiplication of x[i] per y[i]
    snrt_ssr_write(SNRT_SSR_DM2, SNRT_SSR_1D, intermidiate_dot + offset); //ft2->intermidiate_dot

    *start_cycle = snrt_mcycle();
    // Enable the SSRs
    snrt_ssr_enable();
    
    // Assembly code to do ft2 = ft0*ft1 + ft2
    asm volatile(
        "frep.o %[n_frep], 1, 0, 0 \n"
        "fmul.d ft2, ft0, ft1\n"
        :
        : [ n_frep ] "r"(chunk_per_core - 1)
        : "ft0", "ft1", "ft2", "memory");

    // Disable SSRs
    snrt_ssr_disable();

    // Read the intermidiate dot products, reconfigure the stream DM2 to be a read stream
    // Use ft0 and stream DM0 as using DM2 could create issues in the FIFO buffer for the write 
    // back of results
    snrt_ssr_loop_1d(SNRT_SSR_DM0, chunk_per_core, sizeof(double));
    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, intermidiate_dot + offset); //ft2->intermidiate_dot

    snrt_ssr_enable();
    
    // load 0 in ft3 and add ft0
    asm volatile(
        "fcvt.d.w ft3, x0\n"
        "frep.o %[n_frep], 1, 0, 0 \n"
        "fadd.d ft3, ft3, ft0\n"
        :
        : [ n_frep ] "r"(chunk_per_core - 1)
        : "ft0", "ft3", "memory");

    snrt_ssr_disable();

    // Copy back the results to dot_core
    asm volatile("fsd ft3, (%0)" :: "r"(dot_core) : "memory");

    // Fence for FPU syncronization
    snrt_fpu_fence();
    
    *end_cycle = snrt_mcycle();
    return;
}