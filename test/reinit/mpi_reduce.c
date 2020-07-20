#include <stdio.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <unistd.h>

int arr[] = { 1, 2, 3, 4, 5, 6 };
int n_local = 3;
int n_all = 6;

int resilient_main(int argc, char *argv[], OMPI_reinit_state_t state)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    do {
        int i, local_sum = 0, global_sum;
        for(i=0; i<n_local; i++)
            local_sum += arr[i];
        printf("rank %d local sum %d\n", rank, local_sum);

        MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if(rank == 0) {
            printf("rank 0 global sum %d\n", global_sum);
            sleep(1);
        }

        MPI_Barrier(MPI_COMM_WORLD);

    } while(1);

    return 0;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    OMPI_Reinit(argc, argv, resilient_main);

    MPI_Finalize();
}
