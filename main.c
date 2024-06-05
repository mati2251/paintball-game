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
int K = 3; // critical section capacity
int local_clock = 0;
int state = 0; // 0 - PLAN, 1 - HELD, 2 - RELEASED

int request_pair_queue_size;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_response_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pair_response_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pair_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t access_cond = PTHREAD_COND_INITIALIZER;

struct request_queue req_queue;

void initialize_request_queue(int capacity) {
    req_queue.requests = (struct request *)malloc(capacity * sizeof(struct request));
    req_queue.size = 0;
    req_queue.capacity = capacity;
}

void enqueue_request(struct request req) {
    if (req_queue.size == req_queue.capacity) {
        req_queue.capacity *= 2;
        req_queue.requests = (struct request *)realloc(req_queue.requests, req_queue.capacity * sizeof(struct request));
    }
    req_queue.requests[req_queue.size++] = req;
}

void dequeue_request(int index) {
    for (int i = index; i < req_queue.size - 1; i++) {
        req_queue.requests[i] = req_queue.requests[i + 1];
    }
    req_queue.size--;
}

int compare_requests(const void *a, const void *b) {
    struct request *req_a = (struct request *)a;
    struct request *req_b = (struct request *)b;
    if (req_a->timestamp == req_b->timestamp) {
        return req_a->rank - req_b->rank;
    }
    return req_a->timestamp - req_b->timestamp;
}

void sort_request_queue() {
    qsort(req_queue.requests, req_queue.size, sizeof(struct request), compare_requests);
}

void request_access_to_critical_section() {
    pthread_mutex_lock(&state_mutex);
    state = 0; // PLAN
    pthread_mutex_unlock(&state_mutex);

    pthread_mutex_lock(&clock_mutex);
    local_clock++;
    int timestamp = local_clock;
    pthread_mutex_unlock(&clock_mutex);

    struct request req = {timestamp, rank};
    enqueue_request(req);

    for (int i = 0; i < size; i++) {
        if (i != rank) {
            send_packet_with_data(rank, i, REQUEST, timestamp);
        }
    }

    pthread_mutex_lock(&pair_response_mutex);
    while (response_count < size - K) {
        pthread_cond_wait(&access_cond, &pair_response_mutex);
    }
    pthread_mutex_unlock(&pair_response_mutex);

    pthread_mutex_lock(&state_mutex);
    state = 1; // HELD
    pthread_mutex_unlock(&state_mutex);
}

void release_critical_section() {
    pthread_mutex_lock(&state_mutex);
    state = 2; // RELEASED
    pthread_mutex_unlock(&state_mutex);

    for (int i = 0; i < req_queue.size; i++) {
        send_packet(rank, req_queue.requests[i].rank, REPLY);
    }
    req_queue.size = 0;
}

void gun_search() {
    request_access_to_critical_section();
    println("%d obtained a gun.", rank);
}


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

    initialize_request_queue(size);

    pthread_create(&thread, NULL, replay_thread, NULL);
    for (int i = 0; i < CYCLE_SIZE; i++) {
        int my_pair = pair();
        gun_search();
        battle(my_pair);
        release_critical_section();
        println("%d released the gun.", rank);
    }

    println("My score is %d", score);
    brodcast_packet_with_data(END_REQUEST, rank, size, score);
    void *ret;
    pthread_join(thread, ret);
    MPI_Finalize();
    return 0;
}
