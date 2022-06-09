#ifndef SPX_TRADER_H
#define SPX_TRADER_H

#include "spx_common.h"

#define BACK_OFF_MAX (8000000000)
#define BACK_OFF_START (5)

int open_trader_read(int trader_id);
int open_trader_write(int trader_id);
dyn_array* create_dynamic_order_array();
void cancel_order(int id, int write_fd);
void amend_order(int id, int quantity, int price, int write_fd);
void place_buy_order(int order_id, char* product, int quantity, int price, int write_fd);
void place_sell_order(int order_id, char* product, int quantity, int price, int write_fd);

void disconnect_trader(int write_fd, int read_fd);
int interpret_exchange_message(char buffer[BUFFER_SIZE], int write_fd, int read_fd);
int send_exchange_message(int* order_no, int write_fd, int read_fd);

struct order {
   char* type; 
   int order_id;
   char* product;
   long quantity;
   long price;
};

typedef struct order order;

int add_order(dyn_array* order_array, order* new_order);

void cleanup(dyn_array* orders_to_send);
int record_exchange_message(char buffer[BUFFER_SIZE], dyn_array* orders_to_send, int order_position, int write_fd, int read_fd);
int add_order(dyn_array* order_array, order* new_order);
dyn_array* create_dynamic_order_array();
int place_order(dyn_array* orders_to_send, int order_position, int write_fd, int read_fd);

#endif