#pragma once


#ifdef DEBUG
#define debug(FORMAT, ...)                                                     \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                    \
         (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank,    \
         lamport_clock, ##__VA_ARGS__, 27, 0, 37);
#define debug_rank()                                                   \
  printf("%c[%d;%dm [%d][🕒%d]: ", 27,                    \
         (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank,    \
         lamport_clock);
#define debugf(FORMAT, ...) printf(FORMAT, ##__VA_ARGS__);
#else
#define debug(...) ;
#define debug_rank() ;
#define debugf(...) ;
#endif

#define println(FORMAT, ...)                                                   \
  printf("%c[%d;%dm [%d][🕒%d]: " FORMAT "%c[%d;%dm\n", 27,                    \
         (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank,    \
         lamport_clock, ##__VA_ARGS__, 27, 0, 37);

#define print_rank(FORMAT, ...)                                                   \
  printf("%c[%d;%dm [%d][🕒%d]:", 27,                    \
         (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank,    \
         lamport_clock);


