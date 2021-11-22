#include <stdio.h>
#include <math.h>
#include <omp.h>

#define N 2000000
#define NUM_THREADS 32

int vsoteDeljiteljev[N];// vsota deljiteljev za posamezno število
int vsota = 0;          // vsota vseh parov prijateljskih števil

int vsotaDeljiteljev(int a);

int main(int argc, char *argv[]) {
    // izračunaj vsote deljiteljev za števila
    for (int i = 0; i < N; i++) {
        vsoteDeljiteljev[i] = 0;
    }

    // ključavnica init
    omp_lock_t sum_lock;
    omp_init_lock(&sum_lock);

    // set število niti
    omp_set_num_threads(NUM_THREADS);

    // paralelno računaj vsoto
    #pragma omp parallel for schedule(guided, 1)
    for (int i = 1; i < N; i++) {
        // vsota števil prvega števila
        int si;
        if (vsoteDeljiteljev[i] != 0) {
            si = vsoteDeljiteljev[i];
        } else {
            si = vsotaDeljiteljev(i);
            vsoteDeljiteljev[i] = si;
        }

        // prijateljskega para znotraj omejitev ne bomo našli
        if (si > N) continue;

        // vsota števil drugega števila
        int ssi;
        if (vsoteDeljiteljev[si] != 0) {
            ssi = vsoteDeljiteljev[si];
        } else {
            ssi = vsotaDeljiteljev(si);
            vsoteDeljiteljev[si] = ssi;
        }

        // sta prijateljski števili
        if (i == ssi && i < si) {
            //printf("%d %d\n", i, si);
            omp_set_lock(&sum_lock);
            vsota += i + si;
            omp_unset_lock(&sum_lock);
        }
    }

    // uniči ključavnico
    omp_destroy_lock(&sum_lock);

    // izpiši vsoto
    printf("Vsota: %d\n", vsota);

    return 0;
}

int vsotaDeljiteljev(int a) {
    int sum = 0;

    for (int i = 2; i < (int) sqrt(a); i++) {
        if (a % i == 0) {
            sum += i;
            sum += a / i;
        }
    }

    // add 1 for number one
    sum += 1;

    // check if sqrt deljitelj
    if (a % ((int) sqrt(a)) == 0) {
        sum += (int) sqrt(a);
    }

    return sum;
}


/*
*   Rezultati izvajanja programa na NSC za N=2000000
*   
*   sekvenčno:      13.398s
*   
*   +----------+-------------+-------------------+------------------+
*   | SCHEDULE | NUM_THREADS | ČAS IZVAJANJA [S] |    POHITRITEV    |
*   +----------+-------------+-------------------+------------------+
*   |          |             |                   |                  |
*   | static   | 2           | 6.829             | 1.96192707570655 |
*   |          |             |                   |                  |
*   | static   | 4           | 3.523             | 3.80300879931876 |
*   |          |             |                   |                  |
*   | static   | 8           | 1.866             | 7.18006430868167 |
*   |          |             |                   |                  |
*   | static   | 16          | 1.371             | 9.77242888402626 |
*   |          |             |                   |                  |
*   | static   | 32          | 1.216             | 11.0180921052632 |
*   |          |             |                   |                  |
*   | dynamic  | 2           | 7.006             | 1.91236083357122 |
*   |          |             |                   |                  |
*   | dynamic  | 4           | 3.761             | 3.56235043871311 |
*   |          |             |                   |                  |
*   | dynamic  | 8           | 1.889             | 7.0926416093171  |
*   |          |             |                   |                  |
*   | dynamic  | 16          | 1.32              | 10.15            |
*   |          |             |                   |                  |
*   | dynamic  | 32          | 1.273             | 10.5247446975648 |
*   |          |             |                   |                  |
*   | dynamic  | 2           | 6.798             | 1.97087378640777 |
*   |          |             |                   |                  |
*   | dynamic  | 4           | 3.483             | 3.84668389319552 |
*   |          |             |                   |                  |
*   | dynamic  | 8           | 1.806             | 7.41860465116279 |
*   |          |             |                   |                  |
*   | dynamic  | 16          | 1.252             | 10.7012779552716 |
*   |          |             |                   |                  |
*   | dynamic  | 32          | 1.212             | 11.0544554455446 |
*   +----------+-------------+-------------------+------------------+
*
*/