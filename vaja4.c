#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ELEMENTS 1000000

void* threadJob(void* x);
int sort(int num_threads);

int elements[NUM_ELEMENTS];
pthread_barrier_t barrier;

struct ThreadData {
    int first_el;
    int last_el;
};

int main(int argc, char const *argv[])
{
    srand(time(NULL));

    // izmeri čas izvajanja za 1,2,4,8 niti
    struct timespec start, finish;

    for (size_t i = 1; i <= 8; i *= 2)
    {
        clock_gettime(CLOCK_BOOTTIME, &start);
        sort(i);
        clock_gettime(CLOCK_BOOTTIME, &finish);

        double elapsed = finish.tv_sec - start.tv_sec;
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("Čas izvajanja z %ld niti: %f\n", i, elapsed);
    }
}

int sort(int num_threads) {
    // napolni random elementov
    for (size_t i = 0; i < NUM_ELEMENTS; i++)
    {
        elements[i] = rand() % 1000;
    }

    // kreiraj niti
    pthread_t thid;
    pthread_barrier_init(&barrier, NULL, num_threads);
    struct ThreadData data_list[num_threads];
    int counter = 0;

    for (int i = 0; i < num_threads; i++)
    {
        // pripravi podatke za nit: elemenit pri katerem začne in pri katerem konča računanje
        data_list[i].first_el = counter;
        counter += (i+1) * NUM_ELEMENTS/num_threads - i * NUM_ELEMENTS/num_threads;

        if (counter >= NUM_ELEMENTS) {
            data_list[i].last_el = NUM_ELEMENTS-1;
        } else {
            data_list[i].last_el = counter;
        }
        
        if (pthread_create(&thid, NULL, threadJob, (void*) &data_list[i]) != 0) {
            perror("Napaka pri ustvarjanju niti:");
            exit(1);
        }
    }

    pthread_join(thid, NULL);

    return 0;
}

void* threadJob(void* d) {
    struct ThreadData* data = (struct ThreadData*) d;
    
    for (size_t i = 0; i < NUM_ELEMENTS; i++)
    {
        // uredi svoj del
        int even = i % 2;
        for (size_t j = data->first_el; j < data->last_el; j+=2) {
            if ((even == 0 && j % 2 == 1) || (even == 1 && j % 2 == 0)) {
                //printf("%ld\n", j);
                j += 1;
            }

            // zamenjaj elementa če nista v pravilnem vrstnem redu
            if (elements[j] > elements[j+1]) {
                int t = elements[j];
                elements[j] = elements[j+1];
                elements[j+1] = t;
            }
        }

        // počakaj ostale niti
        pthread_barrier_wait(&barrier);
    }
}
