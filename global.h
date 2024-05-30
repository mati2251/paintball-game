#pragma once

#define PAIR_REQUEST 0
#define PAIR_RESPONSE 1
#define PAIR_RESULT 2
#define END_REQUEST 3 

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
extern pthread_mutex_t end_mutex;
