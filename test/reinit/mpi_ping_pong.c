#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi-ext.h>
#include <signal.h>
#include <assert.h>

/* Global variable, iteration state */
int iter = -1;

int resilient_main(int argc, char *argv[], OMPI_reinit_state_t state,
        int num_failed_procs, int *failed_procs)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("RESILIENT MAIN!\n");

    char cp_fname[64];
    sprintf(cp_fname, "checkpoint-%d.txt", rank);
    printf("rank=%d fopen %s\n", rank, cp_fname );
    FILE *fp; 

    /* Either RESTARTED or NEW */
    if( OMPI_REINIT_NEW == state ) {
        iter = 0;
        printf("NEW rank=%d\n", rank);
    } else if ( OMPI_REINIT_REINITED == state ) {
        printf("REINITED rank=%d\n", rank);
        fp = fopen(cp_fname, "r");
        assert( NULL != fp );
        fscanf(fp, "%d\n", &iter);
        printf("CHKPT rank=%d iter %d\n", rank, iter);
        fclose(fp);
    }
    else {
        printf("RESPAWNED rank=%d\n", rank);
#if 0
        fp = fopen(cp_fname, "r");
        assert( NULL != fp );
        fscanf(fp, "%d\n", &iter);
        printf("CHKPT rank=%d iter %d\n", rank, iter);
        fclose(fp);
#endif
    }

    MPI_Status status;
    int ret;
    if( rank == 0 ) {
        while( iter < 20 ) {

            iter++;

            ret = MPI_Send(&iter, 1, MPI_INT, !rank, 0, MPI_COMM_WORLD);
            printf("rank=%d ping %d\n", rank, iter);
            fflush(stdout);
            fp = fopen(cp_fname, "w");
            fprintf(fp, "%d\n", iter);
            fclose(fp);
            sleep(1);
        }
    }
    else {
        while( iter < 20 ) {
            ret = MPI_Recv(&iter, 1, MPI_INT, !rank, 0, MPI_COMM_WORLD, &status);
            printf("rank=%d pong %d\n", rank, iter);
            fflush(stdout);

            if( iter == 15 ) {
                printf("KILL rank=%d\n", rank);\
                raise(SIGTERM);
                printf("KILL node rank=%d\n", rank);\
                    kill(getppid(), SIGTERM);
            }
        }
    }

    printf("rank=%d DONE\n", rank);

    return 0;
}

int main(int argc, char *argv[])
{
    int wait = 0;

    // set in gdb!
    while(wait);

    MPI_Init(&argc, &argv);

    OMPI_Reinit( argc, argv, resilient_main );

    MPI_Finalize();

    return 0;
}

