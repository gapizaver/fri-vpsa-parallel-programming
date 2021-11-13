# include <stdio.h>
# include <stdlib.h>
# include <math.h>

double* Random(int n);
double** Matrix(double *A, int n, int r, int c);
double * Max(double* A, int n);

int main(int argc, char* argv[]) {
    //int *p = Random(5);
    int n;
    int r;
    printf("Vnesi n: ");
    fflush(stdout);
    scanf("%d", &n);
    printf("Vnesi r: ");
    fflush(stdout);
    scanf("%d", &r);

    // generiraj random doubles
    printf("1D:\n");
    double *p1 = Random(n);
    for (size_t i = 0; i < n; i++) {
        printf("%f ", p1[i]);
    }
    printf("\n");

    // matrix
    printf("2D:\n");
    int c = (n + (r - 1)) / r;
    double** p2 = Matrix(p1, n, r, c);
    for (size_t i = 0; i < r; i++)
    {
        for (size_t j = 0; j < c; j++) {
            printf("%f ", p2[i][j]);
        }
    }
    printf("\n\n");
    free(p2);
    free(p1);

    // max vrednost
    double* p3 = Max(p1, n);
    printf("Najvecja vrednost: %f na naslovu: %p.\n", *p3, p3);
    return 0;
}

double* Random(int n) {
    double *p = (double *) calloc(n, sizeof(double));

    /* Intializes random number generator */
    time_t t;
    srand((unsigned) time(&t));

    for (size_t i = 0; i < n; i++)
    {
        double r = ((double)rand() / (double)(RAND_MAX));
        p[i] = r;
    }

    return p;
}

double** Matrix(double *A, int n, int r, int c) {
    double** m;
    m = malloc(sizeof(*m) * r);
    printf("el in rov: %d\n", c);

    for (size_t i = 0; i < r; i++)
    {
        m[i] = malloc(sizeof(*m[i]) * c);
        for (size_t j = 0; j < c; j++)
        {
            printf("i:%ld j:%ld\n", i, j);
            if (n-- > 0) {
                //*(*(m + i) + j) = *A;
                m[i][j] = *A;
                A++;
            } else {
                //*(*(m + i) + j) = 0.0;
                m[i][j] = 0.0;
            }
            printf("m[i][j]: %f\n", m[i][j]);
        }
    }

    return m;    
}

double * Max(double* A, int n) {
    double* max = A;
    for (size_t i = 1; i < n; i++)
    {
        if (A[i] > *max) 
        {
            max = &A[i];
        }
    } 

    return max;
}