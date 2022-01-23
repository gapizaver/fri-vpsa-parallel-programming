#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#include "FreeImage.h"

// število barv
#define NUM_COLORS 128
#define NUM_ITERATIONS 10

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Podajte pot do slike kot argument programa\n");
        exit(1);
    }

    // inicializacija generatorja naključnih števil
    time_t t;
    srand((unsigned) time(&t));

    // preberi vhodno sliko iz datoteke
	FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, argv[1], 0);
	// Convert it to a 32-bit image
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);
    

    // dimenzije slike
    int width = FreeImage_GetWidth(imageBitmap32);
    int height = FreeImage_GetHeight(imageBitmap32);
    int pitch = FreeImage_GetPitch(imageBitmap32);
    int img_size = height * pitch;


    // Prepare room for a raw data copy of the image
    unsigned char *image = (unsigned char *)malloc(img_size * sizeof(unsigned char));
	FreeImage_ConvertToRawBits(image, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

    /*
    *  Kompresija slike
    */

    // inicializacija začetnih vrednosti centroidov tako, da jim priredimo naključni vzorec
    // tabela centroidov hrani vrednosti r, g, b 
    unsigned char centroidi[NUM_COLORS][3];
    // tabela c
    int c[width*height];
    // vsote pikslov v gruči
    int sum[NUM_COLORS][3];
    // število pikslov v gručah (za računanje povprečja m);
    int n[NUM_COLORS];

    // init vrednosti
    for (size_t i = 0; i < NUM_COLORS; i++) {
        // pridobi naključni vzorec (njegov index)
        int r = (rand() % (height*width)) * 4;
        centroidi[i][0] = image[r];
        centroidi[i][1] = image[r+1];
        centroidi[i][2] = image[r+2];
        // init vsoto na 0
        sum[i][0] = 0;
        sum[i][1] = 0;
        sum[i][2] = 0;
        n[i] = 0;
    }

    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        for (size_t j = 0; j < width*height; j++) {
            // poišči pikslu najbližji centriod
            int min_dist = -1;       // tranutne minimalna razdalja
            int min_i;          // indeks centrioda s trenutno najmanjšo razdaljo
            // rgb trenutnega piksla
            unsigned char r = image[j*4];
            unsigned char g = image[j*4+1];
            unsigned char b = image[j*4+2];

            // pojdi čez vse centroide
            for (size_t k = 0; k < NUM_COLORS; k++) {
                // rgb trenutnega centroida
                unsigned char rc = centroidi[k][0];
                unsigned char gc = centroidi[k][1];
                unsigned char bc = centroidi[k][2];

                // preveri če evklidska razdalja manjša
                int evklidska_razdalja = sqrt((r-rc)*(r-rc) + (g-gc)*(g-gc) + (b-bc)*(b-bc));
                if (min_dist == -1 || evklidska_razdalja < min_dist) {
                    min_dist = evklidska_razdalja;
                    min_i = k;
                }
            }

            // nastavi indeks centrioda v tabelo c (dodaj piksel v gručo)
            c[j] = min_i;
            // povečaj vsoto pikslov v gruči
            sum[min_i][0] += r;
            sum[min_i][1] += g;
            sum[min_i][2] += b;
            n[min_i]++;
        }

        
        // pojdi čez vse centroide
        for (size_t j = 0; j < NUM_COLORS; j++) {
            // izračunaj povrečje m vseh xi, kjer ci == j
            // če število elementov v gruči je 0 ji dodaj naključni vzorec
            if (n[j] == 0) {
                int r = (rand() % (height*width)) * 4;
                sum[j][0] = image[r];
                sum[j][1] = image[r+1];
                sum[j][2] = image[r+2];
                n[j] = 1;
            }

            if (j == 0) 
                printf("j:%d centriod1 rgb: %d %d %d\n", j, (int)centroidi[j][0], (int)centroidi[j][1], (int)centroidi[j][2]);

            // nova vrednost centrioda je dobljeno povprečje
            centroidi[j][0] = sum[j][0] / n[j];
            centroidi[j][1] = sum[j][1] / n[j];
            centroidi[j][2] = sum[j][2] / n[j];
            
        }

        // sprintaj zaporedno številko iteracije za spremljanje teka programa
        printf("iteracija %d\n", i);
    }

    // elementom priredi vrednost centriodov
    for (size_t i = 0; i < width*height; i++) {
        unsigned char r = centroidi[c[i]][0];
        unsigned char g = centroidi[c[i]][1];
        unsigned char b = centroidi[c[i]][2];

        image[4*i] = r;
        image[4*i+1] = g;
        image[4*i+2] = b;
    }

    // generiraj izhodno sliko
    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image, width, height, pitch,
		32, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, dst, "output.png", 0);

    free(image);

    return 0;
}
