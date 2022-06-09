1. Describe how your exchange works, using diagram(s) if necessary.

My exchange works by using a continuous while true loop that runs in main. 
Within the loop, there is a pause() and if a trader sends a SIGUSR1 signal, 
the signal handler (continue_reading) increments the global variable num_msgs 
and stores the process id of the trader that sent the message. The increment 
of num_msgs signals main to read the message from the trader, and to handle 
the trader message. If a trader disconnects, a SIGCHLD signal would be sent to 
the exchange and in the sighandler, a check_all_disconnected flag would be set 
to true. This will alert the main while loop to set the status of the trader to 
be inactive and to decrement the number of active traders. It then checks if all 
traders have been disconnected, by checking if the number of active traders is 0, 
and if all have disconnected, it will end the program.

2. Describe your design decisions for the trader and how it’s fault-tolerant and efficient. 

In my trader, an array of orders is created to store all the MARKET SELL orders that 
have been sent from the exchange, and there is also an order_position variable that 
stores the position of the latest order that has not yet received an ACCEPTED 
message from the exchange. When the exchange sends an order to the trader, the 
trader will immediately send a matching buy order. Within the main while true loop, 
a time counter is incremented, and when the time counter reaches the current back_off_time 
set, the order is sent again. An order is sent in decreasingly frequent intervals in order 
to not overwhelm the spx_exchange, starting from 8000000000/5, and adding to it ​​8000000000/4, 
8000000000/3 and so on. When an order is accepted, the order_position is incremented, and 
the process is repeated for the new order.


3. Describe your tests and how to run them.
All my tests, unit tests and E2E tests, can be run by running the tests.sh 
file in my tests folder. Please run the relevant make using the makefile from the tests before running. 
For my unit tests, I have mainly tested my helper 
functions, those whose purpose it not to just print or send signals, and 
also my matching order function. For my E2E tests, I have done 
simple tests for BUY, SELL, AMEND and CANCEL for 1 trader, match orders for 1 
trader and match orders for 2 traders.
