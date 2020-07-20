#include <stdio.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <unistd.h>

double value[256*1024] = { 0 };

int resilient_main(int argc, char *argv[], OMPI_reinit_state_t state)
{
    int iterations = 0;
    int max_iterations = 10;
    int rank;
    MPI_Comm world;
    MPI_Comm_dup( MPI_COMM_WORLD, &world );
    //MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_rank(world, &rank);

    do {
        if(rank == 0) {
            //value++;
            //printf("*** rank 0 sets value %d\n", value);
            sleep(1);
        }

        MPI_Bcast(&value[0], 256*1024, MPI_DOUBLE, 0, world);

        printf("rank %d value %d\n", rank, value);

        iterations++;
    } while(iterations < max_iterations);

    return 0;
}

int main(int argc, char *argv[])
{
    int wait = 0;

    while(wait);

    MPI_Init(&argc, &argv);

    OMPI_Reinit(argc, argv, resilient_main);

    MPI_Finalize();
}
