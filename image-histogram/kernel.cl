#define BINS 256

__kernel void histogram(
    __global unsigned char *image_in, 
    __global unsigned int *R,
    __global unsigned int *G,
    __global unsigned int *B,
    int width,
    int height)
{
    // globalna ID
    int i = get_global_id(1);
    int j = get_global_id(0);

    if (i < height && j < width) {
        // lokalna ID
        int locali = get_local_id(1);
        int localj = get_local_id(0);

        // lokalne spremenljivke
        __local unsigned int localR[BINS];
        __local unsigned int localG[BINS];
        __local unsigned int localB[BINS];
        __local int localimax;

        localR[16 * locali + localj] = 0;
        localG[16 * locali + localj] = 0;
        localB[16 * locali + localj] = 0;

        // lokalne niti počakajo prvo nit da nastavi bufferje na 0
        barrier(CLK_LOCAL_MEM_FENCE);

        // napolni lokalne bufferje
        atomic_add(&localR[image_in[(i * width + j) * 4 + 2]], 1);
        atomic_add(&localG[image_in[(i * width + j) * 4 + 1]], 1);
        atomic_add(&localB[image_in[(i * width + j) * 4]], 1);

        // počakaj ostale lokalne niti
        barrier(CLK_LOCAL_MEM_FENCE);

        // seštej vrednosti iz lokalnega bufferja v globalnega (le ena nit)
        if (locali == 0 && localj == 0) {
            for (size_t i = 0; i < BINS; i++) {
                atomic_add(&R[i], localR[i]);
                atomic_add(&G[i], localG[i]);
                atomic_add(&B[i], localB[i]);
            }
        }
    }
}