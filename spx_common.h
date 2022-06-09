#ifndef SPX_COMMON_H
#define SPX_COMMON_H


#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#define FIFO_EXCHANGE "/tmp/spx_exchange_%d"
#define FIFO_TRADER "/tmp/spx_trader_%d"
#define FEE_PERCENTAGE 1
#define PIPE_NAME_SIZE 28
#define BUFFER_SIZE 256
#define ORDER_TYPE_SIZE 7
#define PRODUCT_NAME_SIZE 17
#define ORDER_NAME_SIZE 50
#define DEFAULT_NUM_ORDERS 128


struct dyn_array{
   void** arr;
   int size;
   int capacity;
};

typedef struct dyn_array dyn_array;


#endif

