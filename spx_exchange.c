/**
 * comp2017 - assignment 3
 * Kylie Guo
 * kguo5959
 */


#include "spx_exchange.h"


volatile int trader_msged;
volatile int new_msgs = 0;
volatile bool check_all_disconnected = false;
volatile int trader_disconnected;

/* 
    Signal handler for SIGUSR1, 
    should increment new_msgs to alert main to read new msg
*/
void continue_reading(int sigid, siginfo_t* info, void* context){

    new_msgs += 1;
    trader_msged = info -> si_pid;
}

/* 
    Signal handler for SIGCHLD (if a child process has ended)
    should set flag to check if all traders are disconnected 
*/
void disconnect_trader(int sigid, siginfo_t* info, void* context){

    check_all_disconnected = true;
    trader_disconnected = info -> si_pid;
}


/*
    Frees memory for a list of all the orders 
*/
void free_all_orders(dyn_array* orders){

    if (orders == NULL){
        return;
    }

    for (int i = 0; i < orders -> size; i++){

        dyn_array* cur_product_orders = (orders -> arr)[i];
        free_order_array(cur_product_orders);

    }

    free(orders -> arr);
    free(orders);


}

/*
    Frees memory for a list of orders 
*/
void free_order_array(dyn_array* orders){

    if (orders == NULL){
        return;
    }

    for (int j = 0; j < orders -> size; j++){
        order* cur_order =  ((order**)(orders -> arr))[j];
        free_order(cur_order);
    }

    free(orders -> arr);
    free(orders);

}



/*
    Frees memory for a trader
*/
void free_trader(trader* del_trader){

    if (del_trader == NULL){
        return;
    }

    free(del_trader -> positions);
    free(del_trader-> quantities);
    free((del_trader -> buy_orders) -> arr);
    free(del_trader -> buy_orders);
    free((del_trader -> sell_orders) -> arr);
    free(del_trader -> sell_orders);
    free(del_trader);

}

/*
    Frees memory for a list of products
*/
void free_product_array(dyn_array* products_array){

    if (products_array == NULL){
        return;
    }
    
    for (int i = 0; i < (products_array -> size); i++){
        free((products_array -> arr)[i]);
    }

    free(products_array -> arr);

    free(products_array);

}


/*
    Close and delete FIFOs, and free memory before exiting.
*/
void cleanup(trader** traders, 
            dyn_array* products_array, 
            int num_traders, 
            dyn_array* buy_orders, 
            dyn_array* sell_orders){

    // free memory for buy orders
    free_all_orders(buy_orders);

    // free memory for sell orders
    free_all_orders(sell_orders);

    // free products array
    free_product_array(products_array);
    
    // Close and delete FIFOs
    for (int i = 0; i < num_traders; i++){

        close(traders[i] -> read_fd);
        close(traders[i] -> write_fd);

        char exchange_pipe[PIPE_NAME_SIZE];
        sprintf(exchange_pipe, FIFO_EXCHANGE, i);
        unlink(exchange_pipe);

        char trader_pipe[PIPE_NAME_SIZE];
        sprintf(trader_pipe, FIFO_TRADER, i);
        unlink(trader_pipe);

        free_trader(traders[i]);
    }

    free(traders);


}


/*
    Creates a dynamic array of products
*/
char** create_dynamic_product_array(int size){

    char** out = (char**) malloc(sizeof(char*)*size);

    if (out == NULL){
        return NULL;
    }

    return out;
}


/*
    Creates a dynamic array of orders
*/
dyn_array* create_dynamic_order_array(){

    dyn_array* out = (dyn_array*) malloc(sizeof(dyn_array));

    if (out == NULL){
        return NULL;
    }

    out -> capacity = DEFAULT_NUM_ORDERS;
    out -> size = 0;
    out -> arr = (void*) malloc(sizeof(order*)*DEFAULT_NUM_ORDERS);

    if (out -> arr == NULL){
        return NULL;
    }

    return out;
}

/*
    Adds order to a list of orders
*/
int add_order(dyn_array* order_array, order* new_order){

    if (new_order == NULL || order_array == NULL){
        return -1;
    }

    if (((order_array -> size) + 1) > (order_array -> capacity)){
        order_array -> capacity *= 2;
        order_array -> arr = realloc(order_array -> arr, (order_array -> capacity) * sizeof(order*));

        // check realloc
        if (order_array -> arr == NULL){
            printf("Could not allocate memory for an additional order\n");
            return -1;
        }
    } 

    ((order**)(order_array -> arr))[order_array -> size] = new_order;
    order_array -> size += 1;

    return 0;

}

/*
    Adds order to a list of order and arranges it in price-time priority
*/
int add_order_sort(dyn_array* order_array, order* new_order){

    int out = add_order(order_array, new_order);

    // sort list of orders by price and time
    if (strcmp((new_order -> type), "BUY") == 0){
        qsort(order_array -> arr, order_array -> size, sizeof(order*), buy_price_time_position);
    } else if (strcmp((new_order -> type), "SELL") == 0){
        qsort(order_array -> arr, order_array -> size, sizeof(order*), sell_price_time_position);
    }

    return out;
}

/*
    Comparator for sorting the orders in price-time priority (lowest to highest price for sell)
*/
int sell_price_time_position(const void* a, const void* b) {

    const order* order_1 = *(const order**)(a);
    const order* order_2 = *(const order**)(b);

    if (order_1 -> price < order_2 -> price){
        // order_1 goes first
        return -1;
    } else if (order_1 -> price > order_2 -> price){
        // order_2 goes first
        return 1;
    } else {
            
        if (order_1 -> processed_order < order_2 -> processed_order){
            return -1;
        } else {
            return 1;
        }
    }

}

/*
    Comparator for sorting the orders in price-time priority (highest to lowest price for buy)
*/
int buy_price_time_position(const void* a, const void* b) {

    const order* order_1 = *(const order**)(a);
    const order* order_2 = *(const order**)(b);

    if (order_1 -> price < order_2 -> price){
        // order 2 goes first
        return 1;
    } else if (order_1 -> price > order_2 -> price){
        // order 1 goes first
        return -1;
    } else {

        // order 1 goes first
        if (order_1 -> processed_order < order_2 -> processed_order){
            return -1;
        } else {
            return 1;
        }
    }
        

}

/*
    Remove order from a list of orders, but does not free any memory
*/
int del_order(struct dyn_array* dyn, int index){

    if (dyn == NULL || index >= (dyn -> size) || index < 0 || dyn -> size == 0){
        return -1;
    }

    memmove(&dyn->arr[index], &dyn->arr[index] + 1, (dyn -> size - index) * sizeof(order*));

    dyn -> size -= 1;

    return 0;

}



/*
    Remove order from a list of orders
*/
int remove_order(struct dyn_array* dyn, int index){

    if (dyn == NULL || index >= (dyn -> size) || index < 0 || dyn -> size == 0){
        return -1;
    }

    free_order(((order**)(dyn -> arr))[index]);

    // move every elem after it up
    memmove(&dyn->arr[index], &dyn->arr[index] + 1, (dyn -> size - index) * sizeof(order*));

    dyn -> size -= 1;

    return 0;

}

/*
    Adds a product to a list of products
*/
void add_product(char** product_array, char* product, int product_id){

    if (product_array == NULL || product == NULL){
        return;
    }

    // allocate space for the product string + null terminator
    product_array[product_id] = (char*) malloc(sizeof(char)*(strlen(product) + 1));

    // check malloc
    if (product_array[product_id] == NULL){
        return;
    }

    strcpy((product_array[product_id]), product);

    return;
}


/*
    Helper function to check what a string starts with
*/
bool starts_with(const char* str, const char* pre){
    
    if (str == NULL || pre == NULL){
        return false;
    }

    return strncmp(pre, str, strlen(pre)) == 0;
}

/*
    Helper function to check what a string ends with
*/
bool ends_with(const char* str, const char* suf){

    if (str == NULL || suf == NULL){
        return false;
    }

    size_t suf_len = strlen(suf);
    size_t str_len = strlen(str);

    return strncmp(suf, str + str_len - suf_len, suf_len) == 0;
}

/*
    Sorts the file and creates a list of products
*/
dyn_array* products_parser(char* filename){

    if (!ends_with(filename, ".txt")){
        return NULL;
    }
    
    FILE* products_file = fopen(filename, "r");

    if (products_file == NULL){
        return NULL;
    }

    char buffer[BUFFER_SIZE];

    int num_items;

    fscanf(products_file, "%d\n", &num_items);

    char** products = create_dynamic_product_array(num_items);

    int i = 0;

    printf("%s Trading %d products:", LOG_PREFIX, num_items);


    while (fgets(buffer, BUFFER_SIZE, products_file) != NULL){
        buffer[strcspn(buffer, "\n")] = 0; // strip the new line character

        // product string has to be less than 16
        if (strlen(buffer) > PRODUCT_NAME_SIZE){
            return NULL;
        }

        add_product(products, buffer, i);

        printf(" %s", products[i]);

        i += 1;
    }
    printf("\n");

    dyn_array* product_array = malloc(sizeof(dyn_array));
    product_array -> arr = (void**)products;
    product_array -> size = num_items;

    fclose(products_file); 

   return product_array;

}

/*
   Create an array to store the position of each product a trader has
*/
void set_trader_positions(trader* cur_trader, int num_products){

    if (cur_trader == NULL){
        return;
    }

    cur_trader -> positions = (long*)malloc(sizeof(long)* num_products);

    if (cur_trader -> positions == NULL){
        return;
    }

    for (int i = 0; i < num_products; i++){
        (cur_trader -> positions)[i] = 0;
    }
}

/*
   Create an array to store the quantity of each product a trader has
*/
void set_trader_quantities(trader* cur_trader, int num_products){

    if (cur_trader == NULL){
        return;
    }

    cur_trader -> quantities = (int*)malloc(sizeof(int)* num_products);

    if (cur_trader -> quantities == NULL){
        return;
    }

    for (int i = 0; i < num_products; i++){
        (cur_trader -> quantities)[i] = 0;
    }
}

/*
   Creates a trader
*/
trader* create_trader(int num_products){

    if (num_products < 0){
        return NULL;
    }

    trader* new_trader = (trader*) malloc(sizeof(trader));

    // check malloc
    if (new_trader == NULL){
        return NULL;
    }

    new_trader -> active = true;
    new_trader -> order_index = -1;
    new_trader -> buy_orders = create_dynamic_order_array();
    new_trader -> sell_orders = create_dynamic_order_array();

    if (new_trader -> buy_orders == NULL || new_trader -> sell_orders == NULL){
        return NULL;
    }

    // each trader stores its position and quantity of a product
    set_trader_positions(new_trader, num_products);
    set_trader_quantities(new_trader, num_products);

    return new_trader;

}


/*
   Creates an array holding all the lists of orders for each product
*/
dyn_array* create_all_orders_array(int num_products){

    if (num_products < 0){
        return NULL;
    }

    dyn_array* all_orders = (dyn_array*) malloc(sizeof(dyn_array));
    dyn_array** arr = (dyn_array**) malloc(sizeof(dyn_array*) * num_products);
    
    // check malloc
    if (all_orders == NULL || arr == NULL){
        return NULL;
    }

    all_orders -> size = num_products;
    all_orders -> arr = (void**)arr;

    for (int i = 0; i < num_products; i++){
        arr[i] = create_dynamic_order_array();
    }

    return all_orders;

}

/*
    Creates a list of traders 
*/
trader** create_traders(int num_traders, int num_products){

    trader** traders = (trader**) malloc(sizeof(trader*) * num_traders);

    for (int i = 0; i < num_traders; i++){
        traders[i] = create_trader(num_products);
    }

    return traders;

}

#ifndef TESTING

int main(int argc, char **argv) {

    // Setup signal handlers

    // Read if SIGUSR1 signal is received
    struct sigaction sigusr1_act = {0};
    sigusr1_act.sa_sigaction = continue_reading;
    sigusr1_act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sigusr1_act, NULL);

    // If a SIGCHLD signal has been received, a child has disconnected
    struct sigaction sigchld_act = {0};
    sigchld_act.sa_sigaction = disconnect_trader;
    sigchld_act.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &sigchld_act, NULL);


    if (argc < 2){
        perror("Command line argument are: ./spx_exchange [product file] [trader 0] [trader 1] ... [trader n]");
        return -1;
    }


    printf("%s Starting\n", LOG_PREFIX);

    int trader_num = 0;

    char* filename = argv[1];
    dyn_array* products_array = products_parser(filename);

    if (products_array == NULL || products_array -> arr == NULL ){
        perror("Invalid products file");
        return -1;
    }

    int num_products = products_array -> size;

    int num_traders = argc - 2;
    int active_traders = num_traders;
    long exchange_fees = 0;
    int processed_order = 0;
    int read_fd;
    int write_fd;

    trader** traders = create_traders(num_traders, num_products);

    dyn_array* buy_orders = create_all_orders_array(num_products);
    dyn_array* sell_orders = create_all_orders_array(num_products);


    // for each trader binary, create 2 fifos
    for (int i = 2; i < argc; i++){

        // Create 2 names pipes for each trader
        char exchange_pipe[PIPE_NAME_SIZE];
        char trader_pipe[PIPE_NAME_SIZE];

        sprintf(exchange_pipe, FIFO_EXCHANGE, trader_num);

        sprintf(trader_pipe, FIFO_TRADER, trader_num);

        // remove the fifo, if it already exists
        if (!access(exchange_pipe, F_OK)){
            unlink(exchange_pipe);
        }

        if (mkfifo(exchange_pipe, 0666) < 0){
            printf("Couldn't make pipe %s\n", exchange_pipe);
            return -1;
        } 

        printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_pipe);

        // remove the fifo, if it already exists
        if (!access(trader_pipe, F_OK)){
            unlink(trader_pipe);
        }


        if (mkfifo(trader_pipe, 0666) < 0){
            printf("Couldn't make pipe %s\n", trader_pipe);
            return -1;
        } 

        printf("%s Created FIFO %s\n", LOG_PREFIX, trader_pipe);


        // launch trader binary as new child process
        int cur_pid = fork();
        (traders[trader_num]) -> pid = cur_pid;
        (traders[trader_num]) -> trader_id = trader_num;

        if (cur_pid < 0){
            perror("Fork failed\n");
            return -1;
        } else if (cur_pid == 0){

            // child process

            char trader_binary[50];
            strcpy(trader_binary, argv[trader_num+2]);
        
            char trader_id[10];
            sprintf(trader_id, "%d", trader_num);
            printf("%s Starting trader %d (%s)\n", LOG_PREFIX, trader_num, argv[trader_num + 2]);

            if (execl(trader_binary, trader_binary, trader_id, NULL) == -1){
                printf("Couldn't launch %s\n", trader_binary);
                cleanup(traders, products_array, num_traders, buy_orders, sell_orders);
                _exit(126);
            }

        } else {
            
            write_fd = open_exchange_write(exchange_pipe);

            if (write_fd < 0){
                fprintf(stderr, "Unable to open exchange pipe (write)\n");
                abort();
                return -1;
            };


            // parent process
            read_fd = open_exchange_read(trader_pipe);

            if (read_fd < 0){
                fprintf(stderr, "Unable to open exchange pipe (read)\n");
                abort();
                return -1;
            }

            open_market(write_fd);
            kill((traders[trader_num]) -> pid, SIGUSR1);

            (traders[trader_num])-> read_fd = read_fd;
            (traders[trader_num]) -> write_fd = write_fd;


            printf("%s Connected to %s\n", LOG_PREFIX, exchange_pipe);

            printf("%s Connected to %s\n", LOG_PREFIX, trader_pipe);

    
        }

        trader_num += 1;

    }

    char buffer[BUFFER_SIZE];
        
    while (true){

        if (check_all_disconnected){

            check_all_disconnected = false;

            trader* disconnected_trader = get_trader(trader_disconnected, traders, num_traders);

            disconnected_trader -> active = false;

            printf("%s Trader %d disconnected\n", LOG_PREFIX, disconnected_trader -> trader_id);

            active_traders -= 1;

            if (active_traders == 0){
                end_program(&exchange_fees);
                break;
            };


        }

        pause();

        if (new_msgs > 0){

            new_msgs -= 1;
            int trader_pid = trader_msged;

            trader* cur_trader = get_trader(trader_pid, traders, active_traders);

            if (cur_trader == NULL){
                continue;
            }

            read_fd = cur_trader -> read_fd;

            if (read(read_fd, buffer, sizeof(buffer)) == -1){
                perror("Problem with reading");
            }


            interpret_trader_message(buffer, buy_orders, sell_orders, 
                                    products_array, cur_trader, traders, 
                                    num_traders, &exchange_fees, &processed_order);
        }



    
    }


    int status;

    // wait until all the child processes have finished before exiting
    for (int i = 0; i < num_traders; i++){
        trader* cur_trader = traders[i];
        waitpid(cur_trader -> pid, &status, 0);
        
    }

    // free all memory, and close pipes
    cleanup(traders, products_array, num_traders, buy_orders, sell_orders);

    return 0;
}

#endif


/*
    Prints out end of exchange message and the amount of exchange fees
*/
void end_program(long* exchange_fees){
    
    printf("%s Trading completed\n", LOG_PREFIX);
    printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, *exchange_fees);

}

/*
    Gets the trader from a list of traders based on its process id
*/
trader* get_trader(int pid, trader** traders, int num_traders){

    if (traders == NULL){
        return NULL;
    }

    for (int i = 0; i < num_traders; i++){
        if (traders[i] -> pid == pid){
            trader* out = traders[i];
            return out;
        }
    }

    return NULL;

}

/*
    Creates an array to store price levels
*/
dyn_array* create_price_level_dyn_array(){

    dyn_array* price_level_arr = (dyn_array*) malloc(sizeof(dyn_array));
    price_level_arr -> capacity = DEFAULT_NUM_ORDERS;
    price_level_arr -> size = 0;
    price_level_arr -> arr = (void**)malloc(sizeof(price_level*) * DEFAULT_NUM_ORDERS);

    return price_level_arr;
}

/*
    Adds a price level to an array of price levels
*/
int add_price_level(dyn_array* price_array, price_level* new_price){

    if (price_array == NULL || new_price == NULL){
        return -1;
    }

    // if full, double size of array
      if (((price_array -> size) + 1) > (price_array -> capacity)){

        price_array -> capacity *= 2;
        price_array -> arr = realloc(price_array -> arr, (price_array -> capacity) * sizeof(price_level*));

        // check realloc
        if (price_array -> arr == NULL){
            printf("Could not allocate memory for an additional order\n");
            return -1;
        }
    } 

    // put the new price level at the last index
    ((price_level**)(price_array -> arr))[price_array -> size] = new_price;
    price_array -> size += 1;

    return 0;
}

/*
    Creates a new price level with a given price, quantity and order type
*/
price_level* create_price_level(long price, long quantity, char* type){

    if (type == NULL){
        return NULL;
    }

    price_level* new_price_level = (price_level*)malloc(sizeof(price_level));

    if (new_price_level == NULL){
        return NULL;
    }

    new_price_level -> price = price;
    new_price_level -> quantity = quantity;
    new_price_level -> orders = 1;

    // allocate space for the type string + null terminator
    new_price_level -> type = (char*) malloc(sizeof(char) * (strlen(type) + 1));

    if (new_price_level -> type == NULL){
        return NULL;
    }

    strcpy(new_price_level -> type, type);

    return new_price_level;
    
}

/*
    Get all the price levels for buy orders in order from highest to lowest
*/
dyn_array* get_buy_price_levels(dyn_array* buy_orders){

    if (buy_orders == NULL){
        return NULL;
    }

    dyn_array* price_levels = create_price_level_dyn_array();

    int cur_price_level = -1;
    price_level* new_price_level;

    // Go through the buy prices from highest to lowest
    for (int i = 0; i < (buy_orders -> size); i++){
        
        int price = ((order**)(buy_orders -> arr))[i] -> price;
        int quantity = ((order**)(buy_orders -> arr))[i] -> quantity;

        if (cur_price_level == price){

            new_price_level -> orders += 1;
            new_price_level -> quantity += quantity;

        } else {

            // create the new price level if the price is different
            new_price_level = create_price_level(price, quantity, "BUY");
            add_price_level(price_levels, new_price_level);
            cur_price_level = price;
        }
    }

    return price_levels;
}

/*
    Get all the price levels for sell orders in order from highest to lowest
*/
dyn_array* get_sell_price_levels(dyn_array* sell_orders){

    if (sell_orders == NULL){
        return NULL;
    }

    int cur_price_level = -1;

    dyn_array* price_levels = create_price_level_dyn_array();

    price_level* new_price_level;
    
    // Since sell orders are arranged from lowest to highest traverse list backwards
    for (int i = (sell_orders -> size) - 1; i >= 0; i--){

        int price = ((order**)(sell_orders -> arr))[i] -> price;
        int quantity = ((order**)(sell_orders -> arr))[i] -> quantity;

        if (cur_price_level == price){
        
            new_price_level -> orders += 1;
            new_price_level -> quantity += quantity;
        
        } else {

            // create the new price level if the price is different
            new_price_level = create_price_level(price, quantity, "SELL");
            add_price_level(price_levels, new_price_level);
            cur_price_level = price;

        }
    }

    return price_levels;
}

/*
    Frees all the memory of a price level
*/
void free_price_levels(dyn_array* price_levels){

    if (price_levels == NULL){
        return;
    }

    for (int i = 0; i < price_levels -> size ; i++){
        free_price_level(((price_level**)(price_levels -> arr))[i]);
    }

    free(price_levels -> arr);
    free(price_levels);

}



/*
    Frees a price level
*/
void free_price_level(price_level* del_price_level){

    free(del_price_level -> type);
    free(del_price_level);
}   


/*
    Handles the reporting of orders
*/
void order_book(order* new_order, 
                dyn_array* buy_orders, 
                dyn_array* sell_orders, 
                int num_traders, 
                trader** traders, 
                dyn_array* product_array){

    int num_products = buy_orders -> size;
    char** product_names = (char**) product_array -> arr;

    printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);

    // print product information
    for (int i = 0; i < num_products; i++){

        dyn_array* cur_product_buy_orders = get_product_orders(buy_orders, i);
        dyn_array* cur_product_sell_orders = get_product_orders(sell_orders, i);

        // Get the different price levels for the orders for a product
        dyn_array* buy_price_levels = get_buy_price_levels(cur_product_buy_orders);
        dyn_array* sell_price_levels = get_sell_price_levels(cur_product_sell_orders);

        // number of price levels 
        int buy_levels = buy_price_levels -> size;
        int sell_levels = sell_price_levels -> size;
        
        printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX,
                                                                    product_names[i],
                                                                    buy_levels,
                                                                    sell_levels);

        // Print prices as levels from highest to lowest

        // if there was a sell price lower than or equal to the highest price, 
        // it would have matched, so print sell prices, then buy prices
        for (int i = 0; i < sell_levels ; i++){

            price_level* cur_price_level = ((price_level**)(sell_price_levels -> arr))[i];

            int size = cur_price_level -> orders;
            int price = cur_price_level -> price;
            int quantity = cur_price_level -> quantity;
            char* type = cur_price_level -> type;
            
            // [SPX]        <ORDER TYPE> <QUANTITY> @$ <PRICE> (<NUMBER OF ORDERS> orders)
            if (size == 0){
      
            } else if (size == 1){
                printf("%s\t\t%s %d @ $%d (1 order)\n", LOG_PREFIX, type, quantity, price);
            } else {
                printf("%s\t\t%s %d @ $%d (%d orders)\n", LOG_PREFIX, type, quantity, price, size);
            }
                
        }

        for (int i = 0; i < buy_price_levels -> size ; i++){

            price_level* cur_price_level = ((price_level**)(buy_price_levels -> arr))[i];

            int size = cur_price_level -> orders;
            int price = cur_price_level -> price;
            int quantity = cur_price_level -> quantity;
            char* type = cur_price_level -> type;
            
            // [SPX]        <ORDER TYPE> <QUANTITY> @$ <PRICE> (<NUMBER OF ORDERS> orders)
            if (size == 0){
      
            } else if (size == 1){
                printf("%s\t\t%s %d @ $%d (1 order)\n", LOG_PREFIX, type, quantity, price);
            } else {
                printf("%s\t\t%s %d @ $%d (%d orders)\n", LOG_PREFIX, type, quantity, price, size);
            }
                
        }

        // free all the price levels array
        free_price_levels(buy_price_levels);
        free_price_levels(sell_price_levels);


    }
    
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);

    // Print the positions of the trader
    for (int i = 0; i < num_traders; i++){

        // [SPX]   Trader <TRADER_ID>: <PRODUCT> <QUANTITY OF PRODUCT> ($<POSITION OF PRODUCT> )
        printf("%s\tTrader %d:", LOG_PREFIX, i);

        trader* cur_trader = traders[i];

        for (int j = 0; j < num_products; j++){

            if (j + 1 == num_products){
                // last product, no comma, new line
                printf(" %s %d ($%ld)\n", product_names[j], 
                                            cur_trader -> quantities[j], 
                                            cur_trader -> positions[j]);
            } else {
                printf(" %s %d ($%ld),", product_names[j], 
                                            cur_trader -> quantities[j], 
                                            cur_trader -> positions[j]);
            }
            
        }
        
    }

}

/*
    Get the position of a product in the list of products
*/
int get_product_position(dyn_array* products_array, order* new_order){

    if (products_array == NULL || new_order == NULL){
        return -1;
    }

    for (int i = 0; i < products_array -> size; i++){

        // if the product names match, return the position
        if (strcmp((products_array -> arr)[i], new_order -> product) == 0){
            return i;
        }

    }

    return -1;
}


/*
    Get the list of orders for a specific product
*/
dyn_array* get_product_orders(dyn_array* all_product_orders, int product_position){

    if (all_product_orders == NULL){
        return NULL;
    }

    if (product_position >= 0 && product_position < all_product_orders -> size){
        return (all_product_orders -> arr)[product_position];
    }

    return NULL;

}

/*
    Frees the memory in an order
*/
void free_order(order* order){

    if (order == NULL){
        return;
    }

    free(order -> type);
    free(order -> product);
    free(order -> trader);
    free(order); 
}


/*
    Command should only be read up to ;
*/
void check_command(char buffer[BUFFER_SIZE]){

    char stripped[BUFFER_SIZE] = {0};

    for (int i = 0; i < strlen(buffer); i++){

        // stop reading at ;
        if (buffer[i] == ';'){
            strncpy(stripped, buffer, i);
            stripped[i+1] = '\0'; // add null terminator
            strcpy(buffer, stripped);
            return;
        }
    }

}

/*
    Allocate memory for a new order 
*/
order* create_order(){
    order* new_order = (order*) malloc(sizeof(order));
    new_order -> type = (char*) malloc(sizeof(char)*ORDER_TYPE_SIZE);
    new_order -> product = (char*) malloc(sizeof(char)*PRODUCT_NAME_SIZE);
    new_order -> trader = (trader**) malloc(sizeof(trader*));

    return new_order;
}

/*
    Handles receiving an order
*/
int interpret_trader_message(char buffer[BUFFER_SIZE],
                                    dyn_array* buy_orders,
                                    dyn_array* sell_orders,
                                    dyn_array* products_array,
                                    trader* cur_trader,
                                    trader** traders, 
                                    int num_traders,
                                    long* exchange_fees,
                                    int* processed_order){

    // read till ;                                    
    check_command(buffer);

    order* new_order = create_order();
    *(new_order -> trader) = cur_trader; 
    new_order -> processed_order = *processed_order; // set time of order
    *processed_order += 1;

    int write_fd = cur_trader -> write_fd;
    int product_position;

    printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX,
                                                cur_trader -> trader_id,
                                                buffer);

    // Scan the command correctly
    if (starts_with(buffer, "CANCEL")){

        sscanf(buffer, "%s %d", new_order -> type, 
                                    &(new_order -> order_id));

    } else if (starts_with(buffer, "AMEND")){
        
         sscanf(buffer, "%s %d %ld %ld", new_order -> type, 
                                    &(new_order -> order_id),
                                    &(new_order -> quantity),
                                    &(new_order -> price));
    } else if (starts_with(buffer, "BUY") || starts_with(buffer, "SELL")){

        sscanf(buffer, "%s %d %s %ld %ld", new_order -> type, 
                                    &(new_order -> order_id),
                                    new_order -> product,
                                    &(new_order -> quantity),
                                    &(new_order -> price));

        
        product_position = get_product_position(products_array, new_order);

        // product doesn't exist
        if (product_position < 0){
            invalid_order(write_fd, new_order);
            return -1;
        }

    } else {
        invalid_order(write_fd, new_order);
        return -1;
    }

    int order_id = new_order -> order_id;
    int pid = cur_trader -> pid;
    
    // Get the relevant list of products for this order
    dyn_array* cur_product_buy_orders = get_product_orders(buy_orders, product_position);
    dyn_array* cur_product_sell_orders = get_product_orders(sell_orders, product_position);

    if (strcmp((new_order -> type), "BUY") == 0){

        if (check_valid_order(new_order)){

            // make sure no order with this id exists for this trader
            if (check_valid_order_id(new_order, cur_product_buy_orders) == -1 &&
                check_valid_order_id(new_order, cur_product_sell_orders) == -1){

                    // add order to list of orders and trader's list of orders
                    add_order_sort(cur_product_buy_orders, new_order);
                    add_order(cur_trader -> buy_orders, new_order);

                    // alert trader that their order has been added
                    accept_order(write_fd, order_id, pid);

                    // alert other traders of new_order
                    inform_other_traders(new_order, traders, num_traders);
                    
                    match_orders(cur_product_buy_orders, cur_product_sell_orders, product_position, exchange_fees);
                    order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);

                    cur_trader -> order_index += 1;

                    return 0;
                }
        } 

        invalid_order(write_fd, new_order);

        return -1;
        
    } else if (strcmp((new_order -> type), "SELL") == 0){

        if (check_valid_order(new_order)){

            // make sure no order with this id exists for this trader
            if (check_valid_order_id(new_order, cur_product_buy_orders) == -1 &&
                check_valid_order_id(new_order, cur_product_sell_orders) == -1){

                    // add order to list of orders and trader's list of orders
                    add_order_sort(cur_product_sell_orders, new_order);
                    add_order(cur_trader -> sell_orders, new_order);

                    // alert trader that their order has been added
                    accept_order(write_fd, order_id, pid);

                    // alert other traders of new_order
                    inform_other_traders(new_order, traders, num_traders);

                    match_orders(cur_product_buy_orders, cur_product_sell_orders, product_position, exchange_fees);
                    order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);

                    cur_trader -> order_index += 1;
                    
                    return 0;
                }
        } 
        
        invalid_order(write_fd, new_order);
        return -1;

    } else if (strcmp((new_order -> type), "AMEND") == 0){


        // check order exists for trader
        if (check_valid_order(new_order) 
            && new_order -> order_id <= cur_trader -> order_index){

            order* og_order;

         
            if ((og_order = get_order(new_order -> order_id, cur_trader -> buy_orders)) != NULL){

                // get the position of the original order in the list of orders
                product_position = get_product_position(products_array, og_order);
                cur_product_buy_orders = get_product_orders(buy_orders, product_position);
                cur_product_sell_orders = get_product_orders(sell_orders, product_position);

                // get info from old order
                amend_order_info(new_order, og_order);

                // remove the order from the list of orders and trader's list of orders
                del_order(cur_trader -> buy_orders, get_order_position(og_order, cur_trader -> buy_orders));
                remove_order(cur_product_buy_orders, get_order_position(og_order, cur_product_buy_orders));
                
                // add it back
                add_order_sort(cur_product_buy_orders, new_order);
                add_order(cur_trader -> buy_orders, new_order);

                // alert trader that their order has been amended
                amend_order(write_fd, order_id, pid);

                // alert other traders of amended order
                inform_other_traders(new_order, traders, num_traders);

                match_orders(cur_product_buy_orders, cur_product_sell_orders, product_position, exchange_fees);
                order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);

                return 0;
             } else if ((og_order = get_order(new_order -> order_id, cur_trader -> sell_orders)) != NULL){

                // get the position of the original order in the list of orders
                product_position = get_product_position(products_array, og_order);
                cur_product_buy_orders = get_product_orders(buy_orders, product_position);
                cur_product_sell_orders = get_product_orders(sell_orders, product_position);

                // get info from old order
                amend_order_info(new_order, og_order);

                // remove the order from the list of orders and trader's list of orders
                del_order(cur_trader -> buy_orders, get_order_position(og_order, cur_trader -> sell_orders));
                remove_order(cur_product_sell_orders, get_order_position(og_order, cur_product_sell_orders));

                // add it back
                add_order_sort(cur_product_sell_orders, new_order);
                add_order(cur_trader -> sell_orders, new_order);
                
                // alert trader that their order has been amended
                amend_order(write_fd, order_id, pid);

                // alert other traders of amended order
                inform_other_traders(new_order, traders, num_traders);

                match_orders(cur_product_buy_orders, cur_product_sell_orders, product_position, exchange_fees);
                order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);
                
                return 0;
             }
        } 
        
        invalid_order(write_fd, new_order);

        return -1;
    } else if (strcmp((new_order -> type), "CANCEL") == 0){

        order* og_order;

        if (new_order -> order_id > cur_trader -> order_index){
            
            invalid_order(write_fd, new_order);
            return -1;

        } else if ((og_order = get_order(new_order -> order_id, cur_trader -> buy_orders)) != NULL){
            
            // get the position of the original order in the list of orders
            product_position = get_product_position(products_array, og_order);
            cur_product_buy_orders = get_product_orders(buy_orders, product_position);

            // get info from original order
            cancel_order_info(new_order, og_order);

            // remove the order from the list of orders and trader's list of orders
            del_order(cur_trader -> buy_orders, get_order_position(og_order, cur_trader -> buy_orders));
            remove_order(cur_product_buy_orders, get_order_position(og_order, cur_product_buy_orders));
            

            // alert trader that their order has been cancelled
            cancel_order(write_fd, order_id, pid);

            // alert other traders of cancelled order
            inform_other_traders(new_order, traders, num_traders);

            order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);
        
            free_order(new_order);

            return 0;
        } else if ((og_order = get_order(new_order -> order_id, cur_trader -> sell_orders)) != NULL){

            // get the position of the original order in the list of orders
            product_position = get_product_position(products_array, og_order);
            cur_product_sell_orders = get_product_orders(sell_orders, product_position);

            // get info from original order
            cancel_order_info(new_order, og_order);

            // remove the order from the list of orders and trader's list of orders
            del_order(cur_trader -> sell_orders, get_order_position(og_order, cur_trader -> sell_orders));
            remove_order(cur_product_sell_orders, get_order_position(og_order, cur_product_sell_orders));
            
            // alert trader that their order has been cancelled
            cancel_order(write_fd, order_id, pid);

            // alert other traders of cancelled order
            inform_other_traders(new_order, traders, num_traders);

            order_book(new_order, buy_orders, sell_orders, num_traders, traders, products_array);

            free_order(new_order);

            return 0;

        }

        invalid_order(write_fd, new_order);

        return -1;

    } else {

        invalid_order(write_fd, new_order);

        return -1;
    }


}


/*
    Copies over the type and product of a cancelled order to a new order 
    and sets the price and quantity to 0
*/
void cancel_order_info(order* new_order, order* og_order){

    if (new_order == NULL || og_order == NULL){
        return;
    }

    strcpy(new_order -> type, og_order -> type);
    strcpy(new_order -> product, og_order -> product);
    new_order -> price = 0;
    new_order -> quantity = 0;
}


/*
    Copies over the type and product of an amended order to a new order
*/
void amend_order_info(order* new_order, order* og_order){

    if (new_order == NULL || og_order == NULL){
        return;
    }

    strcpy(new_order -> type, og_order -> type);
    strcpy(new_order -> product, og_order -> product);

}

/*
    Checks if an order exists in the list of orders
    Returns the position of it in the list of orders if it exists
*/
int check_valid_order_id(order* new_order, dyn_array* orders){

    if (new_order == NULL || orders == NULL){
        return -2;
    }

    trader* new_trader = *(new_order -> trader);
    
    // order ids have to be incremental
    if (((new_trader -> order_index) + 1) != new_order -> order_id){
        return -2;
    }

    return get_order_position(new_order, orders);

}


/*
    Get an order from a list of a trader's orders,
    matched based on order_id
*/
order* get_order(int order_id, dyn_array* orders){

    if (orders == NULL || order_id < 0 || order_id > 999999){
        return NULL;
    }

    for (int i = 0; i < orders -> size; i++){

        order* cur_order = ((order**)(orders -> arr))[i];

        if (cur_order -> order_id == order_id){
            return cur_order;
        }
    }

    return NULL;
}


/*
    Get the position of an order in a list of orders
    Order ids are unique for each trader
*/
int get_order_position(order* new_order, dyn_array* orders){

    if (new_order == NULL || orders == NULL){
        return -1;
    }

    trader* new_trader = *(new_order -> trader);

    if (new_trader == NULL){
        return -1;
    }

    for (int i = 0; i < orders -> size; i++){
        order* cur_order = (orders -> arr)[i];
        trader* cur_trader = *(cur_order -> trader);

        if (((cur_order -> order_id) == (new_order -> order_id)) &&
            (cur_trader -> trader_id) == (new_trader -> trader_id)){
                return i;
        }
    }

    return -1;
}


/*
    Match orders in price-time priority
    Given a list of buy and sell orders for a product
*/
bool match_orders(dyn_array* buy_orders, dyn_array* sell_orders, 
                    int product_position, long* exchange_fees){

    // has to be at least one BUY and one SELL order to match
    if (buy_orders == NULL || sell_orders == NULL ||
        buy_orders -> size < 1 || sell_orders -> size < 1){
        return false;
    }
 
    order* highest_buy_order = ((buy_orders -> arr)[0]);
    order* lowest_sell_order = ((sell_orders -> arr)[0]);
    order* new_order;
    order* old_order;

    bool match = false;

    // keep matching as long as there is a buy order with a price >= lowest sell order price
    while (highest_buy_order -> price >= lowest_sell_order -> price){

        long quantity;

        // quantity of transaction is whatever the biggest quantity that can be fulfilled is
        if (highest_buy_order -> quantity > lowest_sell_order -> quantity){
            quantity = lowest_sell_order -> quantity;
        } else {
            quantity = highest_buy_order -> quantity;
        }

        long price;
        long transaction_fee;
        trader* seller = *(lowest_sell_order -> trader);
        trader* buyer = *(highest_buy_order -> trader);

        // matching price is the price of the older order
        if (highest_buy_order -> processed_order < lowest_sell_order -> processed_order){
            price = highest_buy_order -> price;

            new_order = lowest_sell_order;
            old_order = highest_buy_order;

            // if buyer is first, seller has to pay transaction fee
            transaction_fee = charge_transaction_fee(price, quantity, seller, product_position, exchange_fees);

        } else {
            price = lowest_sell_order -> price;

            new_order = highest_buy_order;
            old_order = lowest_sell_order;

            // if seller is first, buyer has to pay transaction fee
            transaction_fee = charge_transaction_fee(price, quantity, buyer, product_position, exchange_fees);
            
        }


        match = true;

        // update traders' positions
        (buyer -> quantities)[product_position] += quantity;
        (seller -> quantities)[product_position] -= quantity;

        long value = quantity * price;
        (buyer -> positions)[product_position] -= value;
        (seller -> positions)[product_position] += value;
        
        // let traders know of match
        successful_match((*(highest_buy_order -> trader)),
                highest_buy_order -> order_id,
                quantity);
        successful_match((*(lowest_sell_order -> trader)), 
                        lowest_sell_order -> order_id,
                        quantity);
        
        printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX,
                                                                                    old_order -> order_id, 
                                                                                    (*(old_order -> trader)) -> trader_id,
                                                                                    new_order -> order_id,
                                                                                    (*(new_order -> trader)) -> trader_id,
                                                                                    value, 
                                                                                    transaction_fee);



        // update the quantities of the orders
        highest_buy_order -> quantity -= quantity;
        lowest_sell_order -> quantity -= quantity;

        // remove the order if its quantity has been fulfilled
        if (highest_buy_order -> quantity <= 0){

            int index = get_order_position(highest_buy_order, buy_orders);

            // remove the order from buy_orders
            remove_order(buy_orders, index);

            // delete the reference to this order in the buyer's list of orders
            del_order(buyer -> buy_orders, index);
        }
        
        if (lowest_sell_order -> quantity <= 0) {

            int index = get_order_position(lowest_sell_order, sell_orders);

            // remove the order from sell_orders
            remove_order(sell_orders, index);

             // delete the reference to this order in the seller's list of orders
            del_order(seller -> sell_orders, index);
        }

        // has to be at least one BUY and one SELL order
        if (buy_orders -> size < 1 || sell_orders -> size < 1){
            break;
        }

        // get the updated top orders
        highest_buy_order = ((buy_orders -> arr)[0]);
        lowest_sell_order = ((sell_orders -> arr)[0]);


    }

    return match;
}

/*
    Charge trader the 1% transaction fee
*/
long charge_transaction_fee(long price, long quantity, trader* trader_charged, int product_position, long* exchange_fees){

    long fee = round(0.01*price*quantity);

    // update the trader's position for that product
    (trader_charged -> positions)[product_position] -= fee;

    // exchange gets 1% transaction fee
    *exchange_fees += fee;   

    return fee;
    
}


/*
    Lets the trader know that their order has been fulfilled 
*/
void successful_match(trader* cur_trader, int order_id, int quantity){

    if (cur_trader -> active == false){
        return;
    }

    char out[BUFFER_SIZE] = {0};
    sprintf(out, "FILL %d %d;", order_id, quantity);

    if (write(cur_trader -> write_fd, out, strlen(out)) == -1){
        perror("fill ");
    };

    kill(cur_trader -> pid, SIGUSR1);
}


/*
   Check the numbers of the order are valid 
*/
bool check_valid_order(order* new_order){
    
    // order id must be between 0 and 999999
    if (new_order -> order_id < 0 || new_order -> order_id > 999999){
        return false;
    } 

    // price must be between 1 and 999999
    if (new_order -> price < 1 || new_order -> price > 999999){
        return false;
    }

    // quantity must be between 1 and 999999
    if (new_order -> quantity < 1 || new_order -> quantity > 999999){
        return false;
    }

    return true;

}

/*
   Inform the other traders in the exchange of a new order
*/
void inform_other_traders(order* order, trader** traders, int num_traders){

    char out[BUFFER_SIZE] = {0};
  
    int price = order -> price;
    int quantity = order -> quantity;

    int cur_trader_id = (*(order -> trader)) -> trader_id;

    // MARKET <ORDER TYPE> <PRODUCT> <QTY> <PRICE>;
    sprintf(out, "MARKET %s %s %d %d;", order -> type, 
                                        order -> product, 
                                        quantity,
                                        price);   
    // process id of the trader                                                                                        
    int trader_pid;

    // file descriptor to write to trader
    int write_fd;

    for (int i = 0; i < num_traders; i++){

        // do not send to the trader who sent this order
        if (i == cur_trader_id){
            continue;
        }

        // if the trader has disconnected, do not send 
        if (traders[i] -> active == false){
            continue;
        }

        trader_pid = traders[i] -> pid;
        write_fd = traders[i] -> write_fd;

        if (write(write_fd, out, strlen(out)) == -1){
            perror("Inform other traders failed");
        };

        // send signal
        kill(trader_pid, SIGUSR1);
    }

}

/*
   Inform trader their order has been amended
*/
void amend_order(int write_fd, int order_id, int pid){

    char out[BUFFER_SIZE] = {0};
    sprintf(out, "AMENDED %d;", order_id);

    if (write(write_fd, out, strlen(out)) == -1){
        perror("Amended order failed");
    }

    kill(pid, SIGUSR1);

}

/*
   Inform trader their order has been amended
*/
void invalid_order(int write_fd, order* d_order){

    char out[BUFFER_SIZE] = {0};
    sprintf(out, "INVALID;");

    if (write(write_fd, out, strlen(out)) == -1){
        perror("invalid failed");
    }

    trader* cur_trader = *(d_order -> trader); 
    kill(cur_trader -> pid, SIGUSR1);

    // free memory for invalid order
    free_order(d_order); 

}


/*
   Inform trader their order has been accepted
*/
void accept_order(int write_fd, int order_id, int pid){

    char out[BUFFER_SIZE] = {0};
    sprintf(out, "ACCEPTED %d;", order_id);

    if (write(write_fd, out, strlen(out)) == -1){
        perror("accepted failed");
    }

    kill(pid, SIGUSR1);
    
}

/*
   Inform trader their order has been cancelled
*/
void cancel_order(int write_fd, int order_id, int pid){

    char out[BUFFER_SIZE] = {0};
    sprintf(out, "CANCELLED %d;", order_id);

    if (write(write_fd, out, strlen(out)) == -1 ){
        perror("cancelled failed");
    }

    kill(pid, SIGUSR1);

}


/*
   Write to trader's pipe that the market is open
*/
void open_market(int write_fd){

    char out[BUFFER_SIZE];
    strcpy(out, "MARKET OPEN;");

    if (write(write_fd, out, strlen(out)) == -1){
        perror("market open failed");
    }

}



/*
   Open pipe to read from trader
*/
int open_exchange_read(char trader_pipe[PIPE_NAME_SIZE]){

    int read_fd = open(trader_pipe, O_RDONLY);
    return read_fd;

}


/*
   Open pipe to write to trader
*/
int open_exchange_write(char exchange_pipe[PIPE_NAME_SIZE]){

    int write_fd = open(exchange_pipe, O_WRONLY);
    return write_fd;

}
