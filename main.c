#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DEBUG
#define debug(FORMAT, ...)                                                     \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                      \
         (1 + (state.rank / 7)) % 2, 31 + (6 + state.rank) % 7, state.rank,                \
         state.lamport_clock, ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

#define println(FORMAT, ...)                                                   \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                      \
         (1 + (state.rank / 7)) % 2, 31 + (6 + state.rank) % 7, state.rank,                \
         state.lamport_clock, ##__VA_ARGS__, 27, 0, 37);
struct process_state {
  int lamport_clock;
  int rank;
  int size;
};

struct process_state state;

int main(int argc, char **argv) {
  state.lamport_clock = 0;
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
    perror(
        "Parallel environment does not provide the required thread support\n");
    MPI_Finalize();
    exit(0);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &state.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &state.size);

  println("Hello, World!");

  MPI_Finalize();
  return 0;
}
