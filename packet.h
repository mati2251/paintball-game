#pragma once
#include <mpi.h>

extern MPI_Datatype MPI_PACKET;
extern int lamport_clock;
extern int rank; 

struct packet {
  int lamport_clock;
  int rank;
  int data;
};

int compare_packet(const void *a, const void *b);

void init_packet_type();

int get_packet(struct packet *message, int tag);

struct packet send_packet_with_data(int from, int to, int tag, int data);

struct packet send_packet(int from, int to, int tag);

struct packet brodcast_packet(int tag, int from, int size); 

void internal_event();
