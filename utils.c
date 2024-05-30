#include "utils.h"
#include "global.h"
#include <stdlib.h>
#include <unistd.h>

int pair() {
  internal_event();
  pthread_mutex_lock(&pair_response_mutex);
  response_count = size;
  pthread_mutex_unlock(&pair_response_mutex);
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
  int index = 0;
  for (int i = request_pair_queue_size - 1; i >= 0; i--) {
    if (request_pair_queue[i].rank == rank) {
      index = i;
    }
  }
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

void battle(int my_pair) {
  internal_event();
  if (my_pair == -1) {
    println("%d is waitnig for him pair. %d is victim", rank, rank)
        pthread_mutex_lock(&pair_mutex);
    pthread_cond_wait(&pair_cond, &pair_mutex);
    pthread_mutex_unlock(&pair_mutex);
  } else {
    println("%d is pared with %d. %d is killer.", rank, my_pair, rank);
    sleep(1);
    internal_event();
    int random = rand() % 2;
    score += random;
    if (random == 1) {
      println("%d defeated %d", rank, my_pair)
    } else {
      println("%d defeated %d", rank, my_pair)
    }
    send_packet_with_data(rank, my_pair, PAIR_RESULT, random);
  }
}
