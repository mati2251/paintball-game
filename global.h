#pragma once

#define PAIR_REQUEST 0
#define PAIR_RESPONSE 1
#define PAIR_RESULT 2
#define END_REQUEST 3 

#define REQUEST 4
#define REPLY 5

#define CYCLE_SIZE 3

#include "packet.h"
#include <pthread.h>

extern MPI_Datatype MPI_PACKET;
extern int lamport_clock;
extern int rank;
extern int size;
extern int score;
extern int end_count;

extern int request_pair_queue_size;
extern struct packet *request_pair_queue;
extern int response_count;
extern struct packet pair_request;

extern pthread_mutex_t queue_mutex;
extern pthread_cond_t pair_response_cond;
extern pthread_mutex_t pair_response_mutex;
extern pthread_mutex_t pair_mutex;
extern pthread_cond_t pair_cond;
extern pthread_mutex_t state_mutex;
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t access_cond;
extern struct request_queue req_queue;

struct request {
    int timestamp;
    int rank;
};

struct request_queue {
    struct request *requests;
    int size;
    int capacity;
};

void initialize_request_queue(int capacity);
void enqueue_request(struct request req);
void dequeue_request(int index);
int compare_requests(const void *a, const void *b);
void sort_request_queue();
void request_access_to_critical_section();
void release_critical_section();
void gun_search();
