#include <stdio.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

int arr[] = { 1, 2, 3, 4, 5, 6 };
int n_local = 3;
int n_all = 6;

int count = 30;

#define REINIT_NEW 1

#if REINIT_NEW
int resilient_foo(int argc, char *argv[], OMPI_reinit_state_t state,
        int num_failed_procs,
        int *failed_procs)
#else
int resilient_foo(int argc, char *argv[], OMPI_reinit_state_t state)
#endif
{
    int rank;
    int nranks;
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Load checkpoint, if it exists
    char filename[64];
    sprintf(filename, "chkp-%d.txt", rank);
    FILE *fp = fopen( filename, "r");
    if( NULL != fp ) {
        fscanf(fp, "%d\n", &count);
        fclose(fp);
    }

#if REINIT_NEW
    if( num_failed_procs ) {
        int i;
        printf("Rank %d FAILED PROCS %d: ", rank, num_failed_procs);
        for(i=0; i<num_failed_procs; i++) {
            printf("%d, ", failed_procs[i]);
        }
        printf("\n");
    }
#endif

    while(count) {
        MPI_Barrier(MPI_COMM_WORLD);

        int i, local_sum = 0, global_sum;
        for(i=0; i<n_local; i++)
            local_sum += arr[i];
        printf("rank %d local sum %d\n", rank, local_sum);

        MPI_Allreduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        printf("rank %d global sum %d\n", rank, global_sum);

        printf("rank %d countdown %d\n", rank, count);
        count--;

        // Save checkpoint
        fp = fopen( filename, "w");
        assert( NULL != fp );
        fprintf(fp, "%d\n", count);
        fclose(fp);
        sleep(1);

        // Inject fault
        if(rank == (nranks - 1) && ( count == 28 ) ) {
            printf("rank %d SIGKILL to GROUP\n", rank); \
                fflush(stdout); \
                kill(getppid(), SIGTERM);
            //printf("======> rank %d SIGKILL me <======\n", rank);\
            raise(SIGKILL);
        }
        
        if( 1 && rank == 0 && ( count == 26 ) ) {
            printf("rank %d SIGKILL to GROUP\n", rank);\
                kill(getppid(), SIGTERM);
            //printf("======> rank %d SIGKILL me <======\n", rank);\
            raise(SIGKILL);
        }
        if( 1 && rank == 4 && ( count == 24 ) ) {
            printf("rank %d SIGKILL to GROUP\n", rank);\
                kill(getppid(), SIGTERM);
            //printf("======> rank %d SIGKILL me <======\n", rank);\
            raise(SIGKILL);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int wait = 0;
    while(wait);

    MPI_Init(&argc, &argv);

    OMPI_Reinit(argc, argv, resilient_foo);

    MPI_Finalize();
}
