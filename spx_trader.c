#include "spx_trader.h"


static int trader_id;
volatile int new_msgs = 0;

/*
    Helper function to check what a string starts with
*/
bool starts_with(const char* str, const char* pre){
    return strncmp(pre, str, strlen(pre)) == 0;
}

/* 
    Signal handler for SIGUSR1, 
    should increment new_msgs to alert main to read new msg
*/
void continue_reading(int sigid, siginfo_t* info, void* context){
    new_msgs += 1;
}

/*
    Creates a dynamic array of orders
*/
dyn_array* create_dynamic_order_array(){

    dyn_array* out = (dyn_array*) malloc(sizeof(dyn_array));

    out -> capacity = DEFAULT_NUM_ORDERS;
    out -> size = 0;
    out -> arr = (void*) malloc(sizeof(order*)*DEFAULT_NUM_ORDERS);

    return out;
}

/*
    Adds order to a list of order
*/
int add_order(dyn_array* order_array, order* new_order){

    // if array full
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

int main(int argc, char ** argv) {


    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    printf("%d\n", getpid());

    trader_id = atoi(argv[1]);

    // register signal handler
    struct sigaction act = {0};
    act.sa_sigaction = continue_reading;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &act, NULL);

    int read_fd = open_trader_read(trader_id);
    if (read_fd < 0){
        fprintf(stderr, "Unable to open trader pipe (read)\n");
        abort();
        return -1;
    }

    int write_fd = open_trader_write(trader_id);

    if (write_fd < 0){
        fprintf(stderr, "Unable to open trader pipe (write)\n");
        abort();
        return -1;
    };


    char buffer[BUFFER_SIZE];
    
    dyn_array* orders_to_send = create_dynamic_order_array();
    int order_position = -1;
    int back_off_start = BACK_OFF_START;
    long back_off_time = 0;
    volatile long time = 0;


    while (true){


        if (new_msgs > 0){

            new_msgs -= 1;

            if (read(read_fd, buffer, BUFFER_SIZE) == -1){
                perror("Problem with reading");
            }
            
            int temp = order_position;
            // record the message
            order_position = record_exchange_message(buffer, orders_to_send, order_position, write_fd, read_fd);

            if (order_position == -2){
                break;
            }

            // if an order has been accepted, move on to the next order, reset time count
            if (temp < order_position){
                back_off_start = BACK_OFF_START;
                back_off_time = BACK_OFF_MAX/back_off_start;
                back_off_start -= 1;
                time = 0;
            }

        }

        // if there is an order to send again, send it 
        if (order_position >= 0 && (order_position < orders_to_send -> size)){

            // if the time threshold has been met, send order again
            if (time >= back_off_time){
                
                // place the order, if returns -1 means that it's time to disconnect
                if (place_order(orders_to_send, order_position, write_fd, read_fd) == -1){
                    break;
                };

                back_off_time += BACK_OFF_MAX/back_off_start;


                back_off_start -= 1;
                if (back_off_start <= 0){
                    back_off_start = 1;
                }
            } 

        }

        
        time += 1;


    }

    
    return 0;
    
    
}



/*
    Records a market sell order from the exchange in a list
*/
int record_exchange_message(char buffer[BUFFER_SIZE], dyn_array* orders_to_send, int order_position, int write_fd, int read_fd){

    if (starts_with(buffer, "MARKET SELL")){

        if (orders_to_send -> size == 0){
            order_position = 0;
        }

        order* new_order = (order*)malloc(sizeof(order));
        new_order -> type = malloc(sizeof(char) * ORDER_TYPE_SIZE);
        new_order -> product = malloc(sizeof(char) * PRODUCT_NAME_SIZE);
    
        add_order(orders_to_send, new_order);

        sscanf(buffer, "MARKET %s %s %ld %ld", new_order -> type, 
                                        new_order -> product,
                                        &(new_order -> quantity),
                                        &(new_order -> price));

        // place the matching buy order
        if (place_order(orders_to_send, order_position, write_fd, read_fd) == -1){
            return -2;
        }

        return order_position;

    } else if ((starts_with(buffer, "ACCEPTED"))){
        order_position += 1;
        return order_position;
    }
    
    return order_position;
}


/*
    Frees all memory
*/
void cleanup(dyn_array* orders_to_send){

    for (int i = 0; i < orders_to_send -> size; i++){
        order* cur_order =(((order**)(orders_to_send -> arr))[i]);
        free(cur_order -> product);
        free(cur_order -> type);
        free(cur_order);
    }

    free(orders_to_send -> arr);
    free(orders_to_send);

}



/*
    Places an order if the order is a sell order
*/
int place_order(dyn_array* orders_to_send, int order_position, int write_fd, int read_fd){

    order* new_order = (orders_to_send -> arr)[order_position];

    if (strcmp(new_order -> type, "SELL") == 0){
        // disconnect if quantity is over 1000
        if (new_order -> quantity >= 1000){
            cleanup(orders_to_send);
            disconnect_trader(write_fd, read_fd);
            return -1;
        }

        place_buy_order(order_position, new_order -> product, new_order -> quantity, new_order -> price, write_fd);
    } 

    return 0;
}




/*
    Open fifo to read from exchange
*/
int open_trader_read(int trader_id){

    char exchange_pipe[PIPE_NAME_SIZE];
    sprintf(exchange_pipe, FIFO_EXCHANGE, trader_id);

    int read_fd = open(exchange_pipe, O_RDONLY);
    return read_fd;

}


/*
    Open fifo to write to exchange 
*/
int open_trader_write(int trader_id){

    char trader_pipe[PIPE_NAME_SIZE];
    sprintf(trader_pipe, FIFO_TRADER, trader_id);

    int write_fd = open(trader_pipe, O_WRONLY);
    return write_fd;

}


/*
    Sends a buy order to exchange
*/
void place_buy_order(int order_id, char* product, int quantity, int price, int write_fd){

    // BUY <ORDER_ID> <PRODUCT> <QTY> <PRICE>;
    char order[ORDER_NAME_SIZE];
    sprintf(order, "BUY %d %s %d %d;", order_id, product, quantity, price);
    write(write_fd, order, strlen(order));

    // tell exchange to read
    kill(getppid(), SIGUSR1);
}




/*
    Closes all pipes
*/
void disconnect_trader(int write_fd, int read_fd){

    close(read_fd);
    close(write_fd);

}



