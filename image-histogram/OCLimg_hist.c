/*
*   
*   MERITVE NA NCS:
*   
*   +─────────────────+──────────────────────+───────────────────────+───────────────────+
*   | Velikost slike  | Izvajanje na CPU[s]  | Izvajanje na GPU [s]  | Pohitritev        |
*   +─────────────────+──────────────────────+───────────────────────+───────────────────+
*   | 640x480         | 0.003022             | 0.001136              | 2.66021126760563  |
*   | 800x600         | 0.004739             | 0.001532              | 3.09334203655352  |
*   | 1600x900        | 0.01432              | 0.003396              | 4.21672555948174  |
*   | 1920x1080       | 0.021156             | 0.004581              | 4.61820563195809  |
*   | 3840x2160       | 0.083017             | 0.015155              | 5.4778620917189   |
*   +─────────────────+──────────────────────+───────────────────────+───────────────────+
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <time.h>
#include "FreeImage.h"

#include "err_handling.h"

#define BINS 256
#define WORKGROUP_SIZE	(16)
#define MAX_SOURCE_SIZE	16384

struct histogram {
	unsigned int *R;
	unsigned int *G;
	unsigned int *B;
};

int printHistogram(struct histogram H) {
	printf("Colour\tNo. Pixels\n");
	for (int i = 0; i < BINS; i++) {
		if (H.B[i]>0)
			printf("%dB\t%d\n", i, H.B[i]);
		if (H.G[i]>0)
			printf("%dG\t%d\n", i, H.G[i]);
		if (H.R[i]>0)
			printf("%dR\t%d\n", i, H.R[i]);
	}

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Podajte pot do slike kot argument programa\n");
        exit(1);
    }
    
    // ščepec preberemo iz datoteke
	char ch;
    int i;
	cl_int ret;

    // Branje datoteke
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernel.cl", "r");
    if (!fp) 
	{
		fprintf(stderr, ":-(#\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
    fclose( fp );

    //Load image from file
	FIBITMAP *imageBitmap = FreeImage_Load(FIF_BMP, argv[1], 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);

    // dimenzije slike
    int width = FreeImage_GetWidth(imageBitmap32);
    int height = FreeImage_GetHeight(imageBitmap32);
    int pitch = FreeImage_GetPitch(imageBitmap32);
    int img_size = height * pitch;
    
    // Prepare room for a raw data copy of the image
    unsigned char *image_in = (unsigned char *)malloc(img_size * sizeof(unsigned char));
	FreeImage_ConvertToRawBits(image_in, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    /*for (size_t i = 0; i < img_size*sizeof(unsigned char); i++) {
        printf("%d %d\n", i, image_in[i]);
    }*/


    // Podatki o platformi
    cl_platform_id	platform_id[10];
    cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
			// max. "stevilo platform, kazalec na platforme, dejansko "stevilo platform
    ERR_CHECK(ret);
	
	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,	
						 device_id, &ret_num_devices);				
			// izbrana platforma, tip naprave, koliko naprav nas zanima
			// kazalec na naprave, dejansko "stevilo naprav
    ERR_CHECK(ret);

    // Kontekst
    cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
			// kontekst: vklju"cene platforme - NULL je privzeta, "stevilo naprav, 
			// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
			// dodatni parametri funkcije, "stevilka napake
    ERR_CHECK(ret);
 
    // Ukazna vrsta
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
			// kontekst, naprava, INORDER/OUTOFORDER, napake
    ERR_CHECK(ret);

	// Delitev dela
    const size_t local_item_size[2] = {WORKGROUP_SIZE, WORKGROUP_SIZE};
	const size_t num_groups[2] = {
        ((pitch - 1) / local_item_size[1] + 1) * local_item_size[1],
        ((height - 1) / local_item_size[0] + 1) * local_item_size[0]
    };

    //Initalize the histogram
    struct histogram H;
	H.B = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.G = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.R = (unsigned int*)calloc(BINS, sizeof(unsigned int));

    // Alokacija pomnilnika na napravi
    cl_mem imgin_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  img_size*sizeof(unsigned char), image_in, &ret);
			// kontekst, na"cin, koliko, lokacija na hostu, napaka	
    ERR_CHECK(ret);
    cl_mem R_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  BINS*sizeof(unsigned int), NULL, &ret);
    cl_mem G_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  BINS*sizeof(unsigned int), NULL, &ret);
    cl_mem B_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  BINS*sizeof(unsigned int), NULL, &ret);
    ERR_CHECK(ret);
  
    // Priprava programa
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&source_str,  
												   NULL, &ret);
			// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
			// stringi so NULL terminated, napaka													
    ERR_CHECK(ret);
 
    // Prevajanje
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, "stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabni"ski argumenti

	// Log
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len);
			// program, "naprava, tip izpisa, 
			// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
    ERR_CHECK(ret);
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
    ERR_CHECK(ret);
	printf("%s\n", build_log);
	free(build_log);

    // "s"cepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "histogram", &ret);
			// program, ime "s"cepca, napaka
    ERR_CHECK(ret);

    // začni z merjenjem časa
    clock_t start = clock();    

    // "s"cepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&imgin_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&R_mem_obj);
    ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&G_mem_obj);
    ret |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&B_mem_obj);
    ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&width);
    ret |= clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&height);
        // "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke
    ERR_CHECK(ret);

	// "s"cepec: zagon
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,						
								 num_groups, local_item_size, 0, NULL, NULL);	
			// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
			// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
    ERR_CHECK(ret);
																						
    // Kopiranje rezultatov
    ret = clEnqueueReadBuffer(command_queue, R_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.R, 0, NULL, NULL);
    ret |= clEnqueueReadBuffer(command_queue, G_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.G, 0, NULL, NULL);
    ret |= clEnqueueReadBuffer(command_queue, B_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.B, 0, NULL, NULL);
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej
    ERR_CHECK(ret);

    // končaj merjenje časa
    clock_t end = clock();

    // Prikaz rezultatov
    printHistogram(H);
    
    // prikaz rezultatov meritve časa
    printf("Čas izvajanja: %.6fs\n", ((double) (end - start)) / CLOCKS_PER_SEC);
    
    // ciscenje
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(imgin_mem_obj);
    ret = clReleaseMemObject(R_mem_obj);
    ret = clReleaseMemObject(G_mem_obj);
    ret = clReleaseMemObject(B_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    ERR_CHECK(ret);

    free(image_in);
    free(H.R);
    free(H.G);
    free(H.B);

    return 0;
}

									
