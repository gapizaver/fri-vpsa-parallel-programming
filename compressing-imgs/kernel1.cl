#define NUM_COLORS 128
#define WORKGROUP_SIZE 16

__kernel void kernel1(
    __global unsigned char *image_in, 
    int width,
    int height,
    __global unsigned char *centroidi_r,
    __global unsigned char *centroidi_g,
    __global unsigned char *centroidi_b,
    __global int *c,
    __global int *sum_r,
    __global int *sum_g,
    __global int *sum_b,
    __global int *n
)
{
    // globalna ID
    int y = get_global_id(1);
    int x = get_global_id(0);

    if (y < height && x < width) {
        // lokalna ID
        int localy = get_local_id(1);
        int localx = get_local_id(0);

        // lokalne spremenljivke
        __local int sum_local_r[NUM_COLORS];
        __local int sum_local_g[NUM_COLORS];
        __local int sum_local_b[NUM_COLORS];
        __local int n_local[NUM_COLORS];

        // prva lokalna nit inicializira lokalne spremenljivke
        if (localx == 0 && localy == 0) {
            for (size_t i = 0; i < NUM_COLORS; i++) {
                sum_local_r[i] = 0;
                sum_local_g[i] = 0;
                sum_local_b[i] = 0;
                n_local[i] = 0;
            }
        }

        // počakaj prvo nit, da inicializira
        barrier(CLK_LOCAL_MEM_FENCE);

        // poišči pikslu najbližji centroid
        int min_dist = -1;              // tranutne minimalna razdalja
        int min_i;                      // indeks centrioda s trenutno najmanjšo razdaljo
        
        // rgb trenutnega piksla
        unsigned char r = image_in[(y*width+x)*4];
        unsigned char g = image_in[(y*width+x)*4+1];
        unsigned char b = image_in[(y*width+x)*4+2];
        
        // pojdi čez vse centroide
        for (size_t k = 0; k < NUM_COLORS; k++) {
            // rgb trenutnega centroida
            unsigned char rc = centroidi_r[k];
            unsigned char gc = centroidi_g[k];
            unsigned char bc = centroidi_b[k];

            // preveri če evklidska razdalja manjša od trenutne najmanjše
            int evklidska_razdalja = sqrt((double)((r-rc)*(r-rc) + (g-gc)*(g-gc) + (b-bc)*(b-bc)));
            if (min_dist == -1 || evklidska_razdalja < min_dist) {
                min_dist = evklidska_razdalja;
                min_i = k;
            }
        }
        
        // nastavi indeks centrioda v tabelo c (dodaj piksel v gručo)
        c[width*y+x] = min_i;

        // z uporabo atomičnih operacij povečaj vsoto pikslov v gruči
        atomic_add(&sum_local_r[min_i], (int)r);
        atomic_add(&sum_local_g[min_i], (int)g);
        atomic_add(&sum_local_b[min_i], (int)b);
        atomic_add(&n_local[min_i], 1);

        // počakaj vse lokalne niti
        barrier(CLK_LOCAL_MEM_FENCE);

        // prva (ena) lokalna nit skopira lokalne spremenljivke v globalne
        if (localy == 0 && localx == 0) {
            for (size_t i = 0; i < NUM_COLORS; i++) {
                atomic_add(&sum_r[i], sum_local_r[i]);
                atomic_add(&sum_g[i], sum_local_g[i]);
                atomic_add(&sum_b[i], sum_local_b[i]);
                atomic_add(&n[i], n_local[i]);
            }
        }
    }
}