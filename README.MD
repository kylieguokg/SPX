# SPX

A marketplace where the exchange matches orders between traders, using processes, signals and interprocess communication. Additionally, the auto-trader is a trader that automatically sends a matching buy order whenever a sell order is placed on the market. 

Non-auto traders are found in the tests folder. 

### How to Compile and Run
Use the make file to compile:   
        `make spx_exchange` to compile the exchange.  
        `make spx_trader` to compile trader.  
        
To run:
  `./spx_exchange [product file] [trader 0] [trader 1] ... [trader n]`
  - where traders are trader binaries e.g. ./trader_a ./trader_b
  - trader binaries can be created from the make file in the tests folder e.g. `make trader_sell`, `make trader_buy`
        
### Commands

A trader can send the following orders to the exchange: 

```
BUY <ORDER_ID> <PRODUCT> <QTY> <PRICE>;      
SELL <ORDER_ID> <PRODUCT> <QTY> <PRICE>;       
AMEND <ORDER_ID> <QTY> <PRICE>;   
CANCEL <ORDER_ID>;
```
 
- BUY: An order to buy a product at or below the specified price, up to the specified quantity.   
- SELL: An order to sell a product at the specified price, up to the specified quantity.   
- AMEND: Update the quantity or price of an existing order, that has yet to be fully filled.   
- CANCEL: Cancel an existing order, that has yet to be fully filled.
