#ifndef SPX_EXCHANGE_H
#define SPX_EXCHANGE_H

#include "spx_common.h"

#define LOG_PREFIX "[SPX]"


struct price_level{
   long price;
   int orders;
   long quantity;
   char* type;
};

typedef struct price_level price_level;

struct trader {
   int trader_id;
   int read_fd;
   int write_fd;
   int pid; // process id
   long* positions;
   int* quantities;
   int order_index;
   dyn_array* buy_orders;
   dyn_array* sell_orders;
   bool active;
};

typedef struct trader trader;

struct order {
   char* type; 
   int order_id;
   trader** trader;
   char* product;
   long quantity;
   long price;
   int processed_order;
};

typedef struct order order;
dyn_array* products_parser(char* filename);

trader** create_traders(int num_traders, int num_products);
trader* get_trader(int pid, trader** traders, int num_traders);
trader* create_trader(int num_products);
void free_trader(trader* del_trader); ///

bool starts_with(const char* str, const char* pre);
bool ends_with(const char* str, const char* suf);

void check_command(char buffer[BUFFER_SIZE]); ///
int interpret_trader_message(char buffer[BUFFER_SIZE],
                                    dyn_array* buy_orders,
                                    dyn_array* sell_orders,
                                    dyn_array* products_array,
                                    trader* cur_trader,
                                    trader** traders, 
                                    int num_traders,
                                    long* exchange_fees,
                                    int* processed_order);

bool check_valid_order(order* new_order); ///
int del_order(struct dyn_array* dyn, int index);
void free_order(order* order); ///
void free_order_array(dyn_array* orders); ///
void free_all_orders(dyn_array* orders); ///
order* get_order(int order_id, dyn_array* orders); ///
int add_order(dyn_array* order_array, order* new_order);
int add_order_sort(dyn_array* order_array, order* new_order);
int check_valid_order_id(order* new_order, dyn_array* orders); ///
void cancel_order_info(order* new_order, order* og_order); ///
void amend_order_info(order* new_order, order* og_order); ///
dyn_array* create_dynamic_order_array(); ///
int get_order_position(order* search_order, dyn_array* orders); ///
int remove_order(struct dyn_array* dyn, int index); ///
order* create_order(); ///
dyn_array* create_all_orders_array(int num_products);

void set_trader_positions(trader* cur_trader, int num_products); ///
void set_trader_quantities(trader* cur_trader, int num_products); ///


char** create_dynamic_product_array(int size);
void free_product_array(dyn_array* products_array);
void add_product(char** product_array, char* product, int product_id); ///
int get_product_position(dyn_array* products_array, order* new_order); ///

int open_exchange_read(char trader_pipe[PIPE_NAME_SIZE]); //xx
int open_exchange_write(char exchange_pipe[PIPE_NAME_SIZE]); //xx
void open_market(int write_fd); //xx
void invalid_order(int write_fd, order* del_order); //xx
void amend_order(int write_fd, int order_id, int pid); //xx
void accept_order(int write_fd, int order_id, int pid); //xx
void cancel_order(int write_fd, int order_id, int pid); //xx
void successful_match(trader* cur_trader, int order_id, int quantity); //xx
void inform_other_traders(order* order, trader** traders, int num_traders); //xx

dyn_array* create_price_level_dyn_array();
price_level* create_price_level(long price, long quantity, char* type); ///
dyn_array* get_buy_price_levels(dyn_array* buy_orders); ///
dyn_array* get_sell_price_levels(dyn_array* sell_orders); ///
void free_price_levels(dyn_array* price_levels); ///
void free_price_level(price_level* del_price_level); ///
int add_price_level(dyn_array* price_array, price_level* new_price); ///


dyn_array* get_product_orders(dyn_array* all_product_orders, int product_position);

int buy_price_time_position(const void* a, const void* b);
int sell_price_time_position(const void* a, const void* b);

bool match_orders(dyn_array* buy_orders, dyn_array* sell_orders, int product_position, long* exchange_fees); ///

long charge_transaction_fee(long price, long quantity, 
                           trader* trader_charged, int product_position, 
                           long* exchange_fees); ///

                           
void end_program(long* exchange_fees); //xx


void order_book(order* new_order, 
                dyn_array* buy_orders, 
                dyn_array* sell_orders, 
                int num_traders, 
                trader** traders, 
                dyn_array* product_array); ///x

#endif
