#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <time.h>
#include "FreeImage.h"

#include "err_handling.h"

#define WORKGROUP_SIZE	(16)
#define MAX_SOURCE_SIZE	16384
#define NUM_ITERATIONS 50

#define NUM_COLORS 128

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Podajte pot do slike kot argument programa\n");
        exit(1);
    }

    // inicializacija generatorja naključnih števil
    time_t t;
    srand((unsigned) time(&t));
    
    // ščepec preberemo iz datoteke
	char ch;
    int i;
	cl_int ret;

    // Branje datoteke
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernel1.cl", "r");
    if (!fp) 
	{
		fprintf(stderr, ":-(#\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
    fclose( fp );

    fp = fopen("kernel2.cl", "r");
    if (!fp) {
		fprintf(stderr, ":-( 2#\n");
        exit(1);
    }
    char *source_str2 = (char*)malloc(MAX_SOURCE_SIZE);
    size_t source_size2 = fread(source_str2, 1, MAX_SOURCE_SIZE, fp);
	source_str2[source_size2] = '\0';
    fclose(fp);

    //Load image from file
	FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, argv[1], 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);

    // atributi
    // dimenzije slike
    int width = FreeImage_GetWidth(imageBitmap32);
    int height = FreeImage_GetHeight(imageBitmap32);
    int pitch = FreeImage_GetPitch(imageBitmap32);
    int img_size = height * pitch;
    // centroidi
    unsigned char* centroidi_r = (unsigned char*) calloc(NUM_COLORS, sizeof(unsigned char));
    unsigned char* centroidi_g = (unsigned char*) calloc(NUM_COLORS, sizeof(unsigned char));
    unsigned char* centroidi_b = (unsigned char*) calloc(NUM_COLORS, sizeof(unsigned char));
    // tabela c
    int* c = (int*) calloc(width*height, sizeof(int));
    // vsote pikslov v gruči
    int* sum_r = (int*) calloc(NUM_COLORS, sizeof(int));
    int* sum_g = (int*) calloc(NUM_COLORS, sizeof(int));
    int* sum_b = (int*) calloc(NUM_COLORS, sizeof(int));
    // število pikslov v gručah (za računanje povprečja m);
    int* n = (int*) calloc(NUM_COLORS, sizeof(int));

    // Prepare room for a raw data copy of the image
    unsigned char *image_in = (unsigned char *)malloc(img_size * sizeof(unsigned char));
	FreeImage_ConvertToRawBits(image_in, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    
    // init centroidi
    for (size_t i = 0; i < NUM_COLORS; i++) {
        // pridobi naključni vzorec (njegov index)
        int r = (rand() % (height*width)) * 4;
        centroidi_r[i] = image_in[r];
        centroidi_g[i] = image_in[r+1];
        centroidi_b[i] = image_in[r+2];
    }

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

	// Delitev dela za kernel1 (grupiranje pikslov)
    const size_t local_item_size[2] = {WORKGROUP_SIZE, WORKGROUP_SIZE};
	const size_t num_groups[2] = {
        ((width - 1) / local_item_size[1] + 1) * local_item_size[1],
        ((height - 1) / local_item_size[0] + 1) * local_item_size[0]
    };

    // delitev dela za kernel2 (računanje povprečne vrednosti)
    const size_t local_item_size2 = 256;
    const size_t num_groups2 = ((NUM_COLORS - 1) / local_item_size2 + 1) * local_item_size2;

    // Alokacija pomnilnika na napravi
    cl_mem imgin_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  img_size*sizeof(unsigned char), image_in, &ret);
			// kontekst, na"cin, koliko, lokacija na hostu, napaka	
    ERR_CHECK(ret);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
									  width*height*sizeof(int), NULL, &ret);
    ERR_CHECK(ret)
    cl_mem centroidi_r_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
									  NUM_COLORS*sizeof(unsigned char), centroidi_b, &ret);
    ERR_CHECK(ret)
    cl_mem centroidi_g_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
									  NUM_COLORS*sizeof(unsigned char), centroidi_g, &ret);
    ERR_CHECK(ret)
    cl_mem centroidi_b_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
									  NUM_COLORS*sizeof(unsigned char), centroidi_b, &ret);
    ERR_CHECK(ret)
    cl_mem sum_r_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
                                    NUM_COLORS*sizeof(int), NULL, &ret);
    ERR_CHECK(ret)
    cl_mem sum_g_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
									  NUM_COLORS*sizeof(int), NULL, &ret);
    ERR_CHECK(ret)
    cl_mem sum_b_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
									  NUM_COLORS*sizeof(int), NULL, &ret);
    ERR_CHECK(ret)
    cl_mem n_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
									  NUM_COLORS*sizeof(int), NULL, &ret);
    ERR_CHECK(ret)
    // Priprava programa1
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&source_str,  
												   NULL, &ret);
			// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
			// stringi so NULL terminated, napaka													
    ERR_CHECK(ret);
    cl_program program2 = clCreateProgramWithSource(context, 1, (const char **)&source_str2,  
												   NULL, &ret);
    ERR_CHECK(ret);
 
    // Prevajanje 1
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, "stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabni"ski argumenti
    // Prevajanje 2
    ret = clBuildProgram(program2, 1, &device_id[0], NULL, NULL, NULL);

	// Log
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len);
			// program, "naprava, tip izpisa, 
			// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
	printf("log1:%s", build_log);
	free(build_log);

    // Log 2
	size_t build_log_len2;
	char *build_log2;
	ret = clGetProgramBuildInfo(program2, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len2);
			// program, "naprava, tip izpisa, 
			// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
    ERR_CHECK(ret);

	build_log2 =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program2, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len2, build_log2, NULL);
    ERR_CHECK(ret);
	printf("log2:%s", build_log2);
    free(build_log2);

    // "s"cepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "kernel1", &ret);
			// program, ime "s"cepca, napaka
    ERR_CHECK(ret);
    // kernel 2: priprava objekta
    cl_kernel kernel2 = clCreateKernel(program2, "kernel2", &ret);
    ERR_CHECK(ret);

    // začni z merjenjem časa
    clock_t start = clock();    

    // kernel1: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&imgin_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&width);
    ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&height);
    ret |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&centroidi_r_mem_obj);
    ret |= clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&centroidi_g_mem_obj);
    ret |= clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&centroidi_b_mem_obj);
    ret |= clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&c_mem_obj);
    ret |= clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&sum_r_mem_obj);
    ret |= clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&sum_g_mem_obj);
    ret |= clSetKernelArg(kernel, 9, sizeof(cl_mem), (void *)&sum_b_mem_obj);
    ret |= clSetKernelArg(kernel, 10, sizeof(cl_mem), (void *)&n_mem_obj);
        // "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke
    ERR_CHECK(ret);
    
    // Zagon ščepcov
    // kernel2: argumenti
    ret |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&centroidi_r_mem_obj);
    ret |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&centroidi_g_mem_obj);
    ret |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), (void *)&centroidi_b_mem_obj);
    ret |= clSetKernelArg(kernel2, 3, sizeof(cl_mem), (void *)&sum_r_mem_obj);
    ret |= clSetKernelArg(kernel2, 4, sizeof(cl_mem), (void *)&sum_g_mem_obj);
    ret |= clSetKernelArg(kernel2, 5, sizeof(cl_mem), (void *)&sum_b_mem_obj);
    ret |= clSetKernelArg(kernel2, 6, sizeof(cl_mem), (void *)&n_mem_obj);
    ret |= clSetKernelArg(kernel2, 7, sizeof(cl_mem), (void *)&imgin_mem_obj);
    ret |= clSetKernelArg(kernel2, 8, sizeof(cl_int), (void *)&width);
    ret |= clSetKernelArg(kernel2, 9, sizeof(cl_int), (void *)&width);

    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        // kernel1 zagon
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,						
                                    num_groups, local_item_size, 0, NULL, NULL);	
                // vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
                // kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
                // dogodki, ki se morajo zgoditi pred klicem
        ERR_CHECK(ret);
        
        // kernel2 zagon
        ret = clEnqueueNDRangeKernel(command_queue, kernel2, 1, NULL,						
                                    &num_groups2, &local_item_size2, 0, NULL, NULL);																		
    }

    // Kopiranje rezultatov
    ret = clEnqueueReadBuffer(command_queue, centroidi_r_mem_obj, CL_TRUE, 0,						
                                NUM_COLORS*sizeof(unsigned char), centroidi_r, 0, NULL, NULL); 
    ret |= clEnqueueReadBuffer(command_queue, centroidi_g_mem_obj, CL_TRUE, 0,						
                            NUM_COLORS*sizeof(unsigned char), centroidi_g, 0, NULL, NULL);
    ret |= clEnqueueReadBuffer(command_queue, centroidi_b_mem_obj, CL_TRUE, 0,						
                            NUM_COLORS*sizeof(unsigned char), centroidi_b, 0, NULL, NULL);
    ret |= clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,						
                            width*height*sizeof(int), c, 0, NULL, NULL);
    ERR_CHECK(ret);
    
    // končaj merjenje časa
    clock_t end = clock();

    // generiranje izhodne slike
    unsigned char *image_out = (unsigned char *)malloc(img_size * sizeof(unsigned char));
    // elementom priredi vrednost centriodov
    for (size_t i = 0; i < width*height; i++) {
        unsigned char r = centroidi_r[c[i]];
        unsigned char g = centroidi_g[c[i]];
        unsigned char b = centroidi_b[c[i]];

        image_out[4*i] = r;
        image_out[4*i+1] = g;
        image_out[4*i+2] = b;
        image_out[4*i+3] = 255;     // alpha
    }
    // generiraj sliko
    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image_out, width, height, pitch,
		32, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, dst, "output.png", 0);

    // prikaz rezultatov meritve časa
    printf("Čas izvajanja: %.6fs\n", ((double) (end - start)) / CLOCKS_PER_SEC);
    
    // ciscenje
    ret = clFlush(command_queue);
    ret |= clFinish(command_queue);
    ret |= clReleaseKernel(kernel);
    ret |= clReleaseProgram(program);
    ret |= clReleaseMemObject(imgin_mem_obj);
    ret |= clReleaseMemObject(c_mem_obj);
    ret |= clReleaseMemObject(centroidi_r_mem_obj);
    ret |= clReleaseMemObject(centroidi_g_mem_obj);
    ret |= clReleaseMemObject(centroidi_b_mem_obj);
    ret |= clReleaseMemObject(n_mem_obj);
    ret |= clReleaseMemObject(sum_r_mem_obj);
    ret |= clReleaseMemObject(sum_g_mem_obj);
    ret |= clReleaseMemObject(sum_b_mem_obj);
    ret |= clReleaseCommandQueue(command_queue);
    ret |= clReleaseContext(context);
    ERR_CHECK(ret);

    free(image_in);
    free(image_out);
    free(c);
    free(n);
    free(centroidi_r);
    free(centroidi_g);
    free(centroidi_b);
    free(sum_g);
    free(sum_r);
    free(sum_b);

    return 0;
}

									
