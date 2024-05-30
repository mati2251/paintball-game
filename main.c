#include "global.h"
#include "log.h"
#include "packet.h"
#include "recv.h"
#include "utils.h"
#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

int rank;
int size;
struct packet *request_pair_queue;
int response_count = 0;
int score = 0;
struct packet pair_request;
int end_count;

int request_pair_queue_size;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_response_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pair_response_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char **argv) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  init_packet_type();
  pthread_t thread;
  if (provided != MPI_THREAD_MULTIPLE) {
    perror(
        "Parallel environment does not provide the required thread support\n");
    MPI_Finalize();
    exit(0);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  println("%d starts work", rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  request_pair_queue = malloc((size * 2) * sizeof(struct packet));
  request_pair_queue_size = 0;
  end_count = size;
  srand(rank);

  pthread_create(&thread, NULL, replay_thread, NULL);
  for (int i = 0; i < CYCLE_SIZE; i++) {
    int my_pair = pair();
    // TODO: gun search
    battle(my_pair);
  }
  
  println("My score is %d", score);
  brodcast_packet_with_data(END_REQUEST, rank, size, score);
  void *ret;
  pthread_join(thread, ret);
  MPI_Finalize();
  return 0;
}
