#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#ifdef DEBUG
#define debug(FORMAT, ...)                                                     \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                    \
         (1 + (state.rank / 7)) % 2, 31 + (6 + state.rank) % 7, state.rank,    \
         state.lamport_clock, ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

#define println(FORMAT, ...)                                                   \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                    \
         (1 + (state.rank / 7)) % 2, 31 + (6 + state.rank) % 7, state.rank,    \
         state.lamport_clock, ##__VA_ARGS__, 27, 0, 37);

struct process_state {
  int lamport_clock;
  int rank;
  int size;
  bool finished;
};

struct packet {
  int lamport_clock;
  int rank;
};

struct process_state state;

int get_message(struct packet *message) {
  MPI_Status status;
  MPI_Recv(message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
           &status);
  state.lamport_clock =
      (message->lamport_clock > state.lamport_clock ? message->lamport_clock
                                                    : state.lamport_clock) + 1;
  return status.MPI_TAG;
}

void *recieve_message_thread() {
  struct packet message;
  while (!state.finished) {
    int tag = get_message(&message);
    debug("Recieved message from %d with tag %d", message.rank, tag);
  }
  return NULL;
}

void send_message(int who, int tag) {
  struct packet message;
  state.lamport_clock++;
  message.lamport_clock = state.lamport_clock;
  message.rank = state.rank;
  MPI_Send(&message, 1, MPI_INT, who, tag, MPI_COMM_WORLD);
  debug("Sent message to %d with tag %d", who, tag);
}

int main(int argc, char **argv) {
  state.lamport_clock = 0;
  state.finished = false;
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  pthread_t thread;
  pthread_create(&thread, NULL, recieve_message_thread, NULL);
  if (provided != MPI_THREAD_MULTIPLE) {
    perror(
        "Parallel environment does not provide the required thread support\n");
    MPI_Finalize();
    exit(0);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &state.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &state.size);

  send_message(0, 0);

  pthread_join(thread, NULL);
  MPI_Finalize();
  return 0;
}
