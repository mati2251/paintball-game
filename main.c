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
int K = 3; // Example value for the capacity of the critical section
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

enum state {PLAN, HELD, RELEASED} state = PLAN;
enum message_type {REQUEST, REPLY}; // Declare enum for message types
int replies_needed;

void send_request();
void send_reply(int dest);
void handle_request(struct packet pkt);
void handle_reply();
void* message_listener(void* arg);

int main(int argc, char **argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    init_packet_type();
    pthread_t thread, listener_thread;
    if (provided != MPI_THREAD_MULTIPLE) {
        perror("Parallel environment does not provide the required thread support\n");
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
    pthread_create(&listener_thread, NULL, message_listener, NULL);

    for (int i = 0; i < CYCLE_SIZE; i++) {
        int my_pair = pair();
        // Request gun access
        send_request();
        pthread_mutex_lock(&pair_response_mutex);
        while (response_count < size - K) {
            pthread_cond_wait(&pair_response_cond, &pair_response_mutex);
        }
        pthread_mutex_unlock(&pair_response_mutex);
        state = HELD;
        println("%d obtained a gun, entering critical section", rank);

        battle(my_pair);

        state = RELEASED;
        pthread_mutex_lock(&queue_mutex);
        for (int j = 0; j < request_pair_queue_size; j++) {
            send_reply(request_pair_queue[j].rank);
        }
        request_pair_queue_size = 0;
        pthread_mutex_unlock(&queue_mutex);
        response_count = 0;
    }

    println("My score is %d", score);
    brodcast_packet_with_data(END_REQUEST, rank, size, score);
    void *ret;
    pthread_join(thread, ret);
    pthread_cancel(listener_thread);
    MPI_Finalize();
    return 0;
}

void send_request() {
    state = PLAN;
    lamport_clock++;
    struct packet pkt = { .rank = rank, .lamport_clock = lamport_clock };
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MPI_Send(&pkt, 1, MPI_PACKET, i, REQUEST, MPI_COMM_WORLD);
        }
    }
    println("%d sent request for a gun", rank);
}

void send_reply(int dest) {
    struct packet pkt = { .rank = rank, .lamport_clock = lamport_clock };
    MPI_Send(&pkt, 1, MPI_PACKET, dest, REPLY, MPI_COMM_WORLD);
    println("%d sent reply to %d", rank, dest);
}

void handle_request(struct packet pkt) {
    pthread_mutex_lock(&queue_mutex);
    if (state == HELD || (state == PLAN && (lamport_clock < pkt.lamport_clock || (lamport_clock == pkt.lamport_clock && rank < pkt.rank)))) {
        request_pair_queue[request_pair_queue_size++] = pkt;
    } else {
        send_reply(pkt.rank);
    }
    pthread_mutex_unlock(&queue_mutex);
}

void handle_reply() {
    pthread_mutex_lock(&pair_response_mutex);
    response_count++;
    if (response_count >= size - K) {
        pthread_cond_signal(&pair_response_cond);
    }
    pthread_mutex_unlock(&pair_response_mutex);
}

void* message_listener(void* arg) {
    MPI_Status status;
    struct packet pkt;
    while (true) {
        MPI_Recv(&pkt, 1, MPI_PACKET, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == REQUEST) {
            handle_request(pkt);
        } else if (status.MPI_TAG == REPLY) {
            handle_reply();
        }
    }
    return NULL;
}
