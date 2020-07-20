#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi-ext.h>

/* Global variable, iteration state */
int iter = -1;

int resilient_main(int argc, char *argv[], OMPI_reinit_state_t state)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("RESILIENT MAIN!\n");

    char cp_fname[64];
    sprintf(cp_fname, "checkpoint-%d.txt", rank);
    printf("rank=%d fopen %s\n", rank, cp_fname );
    FILE *fp = fopen(cp_fname, "r+");

    /* Either RESTARTED or NEW */
    if( iter == -1 ) {
        /* NEW */
        if(NULL == fp) {
            iter = 0;
            printf("NEW rank=%d\n", rank);
            fp = fopen(cp_fname, "w");
        }
        else {
            printf("RESTARTED rank=%d\n", rank);
            fscanf(fp, "%d\n", &iter);
            printf("iter %d\n", iter );
        }
    }
    /* REINITED */
    else {
        printf("REINITED rank=%d\n", rank);
    }

    int start_iter = iter;
    while( iter < (start_iter + 1000000 ) ) {
        printf("iter %d -- limit %d\n", iter, start_iter + 1000000 );
        MPI_Status status;

        iter++;
        int ret;
        int send_iter;
        if(!rank)
            send_iter = iter;
        else
            send_iter = -iter;
        if(!rank)
            ret = MPI_Send(&send_iter, 1, MPI_INT, !rank, 0, MPI_COMM_WORLD);
        printf("rank=%d ping %5d", rank, send_iter);
        rewind(fp);
        fprintf(fp, "%6d\n", iter);
        fflush(fp);

        int n = 0;
        if(rank)
            ret = MPI_Recv(&n, 1, MPI_INT, !rank, 0, MPI_COMM_WORLD, &status);
        printf(" pong %d\n", n);
        sleep(1);
    }

    fclose(fp);

    return 0;
}

int main(int argc, char *argv[])
{
    int wait = 0;

    // set in gdb!
    while(wait) { sleep(1); }

    MPI_Init(&argc, &argv);

    OMPI_Reinit( argc, argv, resilient_main );

    MPI_Finalize();

    return 0;
}

