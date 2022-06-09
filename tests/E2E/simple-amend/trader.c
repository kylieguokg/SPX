#include "../../../spx_trader.h"

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


int main(int argc, char ** argv) {

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

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


    char buffer[BUFSIZ];

    int order_no = 1;

    while (true){

        if (new_msgs > 0){

            new_msgs -= 1;
            if (read(read_fd, buffer, BUFSIZ) == -1){
                perror("Problem with reading");
            }

            if (send_exchange_message(&order_no, write_fd, read_fd) == -1){
                break;
            }


        }



    }

    
    return 0;
    
    
}



int send_exchange_message(int* order_no, int write_fd, int read_fd){


    switch(*order_no)
    {
        // sell order
        case 1:
            sleep(1);
            place_sell_order(0, "MANGO", 10, 10, write_fd);
            sleep(1);
            break;

        // amend order
        case 2:
            sleep(1);
            amend_order(0, 30, 30, write_fd);
            sleep(1);
            break;

        // buy order
        case 3:
            sleep(1);
            place_buy_order(1, "MILK", 100, 10, write_fd);
            sleep(1);
            break;

        // amend order - invalid order_id
        case 4:
            sleep(1);
            amend_order(2, 10, 8, write_fd);
            sleep(1);
            break;

        // amend order - invalid quantity
        case 5:
            sleep(1);
            amend_order(1, 0, 8, write_fd);
            sleep(1);
            break;

        // amend order - invalid quantity
        case 6:
            sleep(1);
            amend_order(1, 1000000, 8, write_fd);
            sleep(1);
            break;

        // amend order - invalid price
        case 7:
            sleep(1);
            amend_order(1, 10, 0, write_fd);
            sleep(1);
            break;

        // amend order - invalid price
        case 8:
            sleep(1);
            amend_order(1, 10, 1000000, write_fd);
            sleep(1);
            break;

        // amend order - valid
        case 9:
            sleep(1);
            amend_order(1, 38, 25, write_fd);
            sleep(1);
            break;


        default:
            disconnect_trader(write_fd, read_fd);
            return -1;

    }

    *order_no += 1;

    return 0;

}

/*
    Open fifo to read from exchange
*/
int open_trader_read(int trader_id){
    
    // tmp/spx_exchange_<Trader ID> 
    char exchange_pipe[PIPE_NAME_SIZE];
    sprintf(exchange_pipe, FIFO_EXCHANGE, trader_id);

    int read_fd = open(exchange_pipe, O_RDONLY);

    return read_fd;

}


/*
    Open fifo to write to exchange 
*/
int open_trader_write(int trader_id){

    // /tmp/spx_trader_<Trader ID>
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
    Sends a sell order to exchange
*/
void place_sell_order(int order_id, char* product, int quantity, int price, int write_fd){

    // SELL <ORDER_ID> <PRODUCT> <QTY> <PRICE>;
    char order[ORDER_NAME_SIZE];
    sprintf(order, "SELL %d %s %d %d;", order_id, product, quantity, price);
    write(write_fd, order, strlen(order));

    // tell exchange to read
    kill(getppid(), SIGUSR1);
}

/*
    Sends an amended order to exchange
*/
void amend_order(int id, int quantity, int price, int write_fd){

    // AMEND <ORDER_ID> <QTY> <PRICE>;
    char order[ORDER_NAME_SIZE];
    sprintf(order, "AMEND %d %d %d;", id, quantity, price);
    write(write_fd, order, strlen(order));

    // tell exchange to read
    kill(getppid(), SIGUSR1);
}

/*
    Sends an cancelled order to exchange
*/
void cancel_order(int id, int write_fd){

    char order[ORDER_NAME_SIZE];
    sprintf(order, "CANCEL %d;", id);
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



