#define NUM_COLORS 128
#define WORKGROUP_SIZE 256

__kernel void kernel2(
    __global unsigned char *centroidi_r,
    __global unsigned char *centroidi_g,
    __global unsigned char *centroidi_b,
    __global int *sum_r,
    __global int *sum_g,
    __global int *sum_b,
    __global int *n,
    __global unsigned char *image_in,
    int width,
    int height
) {
    int id = get_global_id(0);
    
    if (id < NUM_COLORS) {
        // izračunaj povprečno vrednost
        // če število elementov v gruči je 0 ji dodaj naključni vzorec
        if (n[id] == 0) {
            // ne najbolj optimalen način za pridobitev random piksla
            int r = ((id * 9999999) % (height*width)) * 4;
            sum_r[id] = image_in[r];
            sum_g[id] = image_in[r+1];
            sum_b[id] = image_in[r+2];
            n[id] = 1;
        }

        // nova vrednost centrioda je dobljeno povprečje
        centroidi_r[id] = sum_r[id] / n[id];
        centroidi_g[id] = sum_g[id] / n[id];
        centroidi_b[id] = sum_b[id] / n[id];
    }
}