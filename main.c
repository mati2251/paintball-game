#include "log.h"
#include "packet.h"
#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#define PAIR_REQUEST 0
#define PAIR_RESPONSE 1
#define PAIR_RESULT 2
#define CYCLE_SIZE 3

int rank;
int size;
struct packet *request_pair_queue;
int response_count = 0;
struct packet pair_request;

int request_pair_queue_size;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_response_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pair_response_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_cond = PTHREAD_COND_INITIALIZER;

void *replay_thread() {
  struct packet message;
  while (true) {
    int tag = get_packet(&message, MPI_ANY_TAG);
    switch (tag) {
    case PAIR_REQUEST: {
      pthread_mutex_lock(&queue_mutex);
      request_pair_queue[request_pair_queue_size] = message;
      request_pair_queue_size++;
      pthread_mutex_unlock(&queue_mutex);
      send_packet(rank, message.rank, PAIR_RESPONSE);
      debug("Recieve pair request. Sent pair response to %d", message.rank);
      break;
    }
    case PAIR_RESPONSE: {
      pthread_mutex_lock(&pair_response_mutex);
      if (message.lamport_clock < pair_request.lamport_clock) {
        println("Response has lower clock than request %d < %d",
                message.lamport_clock, pair_request.lamport_clock);
        exit(0);
      }
      response_count--;
      pthread_cond_signal(&pair_response_cond);
      debug("Recieve pair response from %d", message.rank);
      pthread_mutex_unlock(&pair_response_mutex);
      break;
    }
    case PAIR_RESULT: {
      pthread_mutex_lock(&pair_mutex);
      println("Recieve result %d from %d", 1 - message.data % 2, message.rank);
      pthread_cond_signal(&pair_cond);
      pthread_mutex_unlock(&pair_mutex);
    }
    }
  }
  return NULL;
}

int pair() {
  internal_event();
  pthread_mutex_lock(&pair_response_mutex);
  response_count = size;
  pthread_mutex_unlock(&pair_response_mutex);
  println("Started pairing");
  pair_request = brodcast_packet(PAIR_REQUEST, rank, size);
  pthread_mutex_lock(&pair_response_mutex);
  while (response_count > 0) {
    pthread_cond_wait(&pair_response_cond, &pair_response_mutex);
  }
  pthread_mutex_unlock(&pair_response_mutex);
  pthread_mutex_lock(&queue_mutex);
  internal_event();
  qsort(request_pair_queue, request_pair_queue_size, sizeof(struct packet),
        compare_packet);
  debug_rank();
  debugf("Ranks %d: ", rank);
  int index = 0;
  for (int i = 0; i < request_pair_queue_size; i++) {
    if (request_pair_queue[i].rank == rank) {
      index = i;
    }
    debugf("%d ", request_pair_queue[i].rank);
  }
  debugf("\n");
  int pair = -1;
  if (index % 2 == 1) {
    pair = request_pair_queue[index - 1].rank;
  }
  int old_size = request_pair_queue_size;
  request_pair_queue_size = request_pair_queue_size % 2;
  if (request_pair_queue_size > 0) {
    request_pair_queue[0] = request_pair_queue[old_size];
  }
  pthread_mutex_unlock(&queue_mutex);
  return pair;
}

int main(int argc, char **argv) {
  request_pair_queue = malloc((size + 1) * sizeof(struct packet));
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
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  srand(rank);

  pthread_create(&thread, NULL, replay_thread, NULL);
  for (int i = 0; i < CYCLE_SIZE; i++) {
    int my_pair = pair();
    if (my_pair != -1) {
      pthread_mutex_lock(&pair_mutex);
      pthread_cond_wait(&pair_cond, &pair_mutex);
      pthread_mutex_unlock(&pair_mutex);
    } else {
      int random = rand() % 2;
      println("My result with %d is %d", my_pair, random)
          send_packet_with_data(rank, my_pair, PAIR_RESULT, random);
    }
    sleep(1);
  }

  MPI_Finalize();
  return 0;
}
