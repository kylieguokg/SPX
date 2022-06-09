#! /usr/bin/env sh

echo "######################"
echo "### Running tests! ###"
echo "######################"

count=0 # number of test cases run so far

gcc -fsanitize=address -g -DTESTING -fprofile-arcs -ftest-coverage -Wall -Werror -Wvla -O0 -std=c11 -lm -fsanitize=address,leak ../spx_exchange.c unit-tests-exchange.c   -L . -l cmocka-static
./a.out

make all 

echo "testing one trader only with only sell orders"
../spx_exchange E2E/one-seller/products.txt ./trader_sell  | diff - E2E/one-seller/one-trader-only-sell.out || echo "One trader only sell: failed!\n"
count=$((count+1))

echo "testing one trader only with only buy orders"
../spx_exchange E2E/one-buyer/products.txt ./trader_buy  | diff - E2E/one-buyer/one-trader-only-buy.out || echo "One trader only buy: failed!\n"
count=$((count+1))

echo "testing one trader only with amended orders"
../spx_exchange E2E/simple-amend/products.txt ./simple-amend | diff - E2E/simple-amend/expected.out || echo "Amend orders: failed!\n"
count=$((count+1))

echo "testing one trader only with cancelled orders"
../spx_exchange E2E/simple-cancel/products.txt ./simple-cancel | diff - E2E/simple-cancel/expected.out || echo "Cancel orders: failed!\n"
count=$((count+1))

echo "testing matching orders with one trader"
../spx_exchange E2E/one-trader-match/products.txt ./one-trader-match  | diff - E2E/one-trader-match/expected.out || echo "Testing matching orders with one trader: failed!\n"
count=$((count+1))

echo "testing matching orders with two traders"
../spx_exchange E2E/two-traders-simple-match/products.txt ./two-traders-simple-match_1 ./two-traders-simple-match_2 | diff - E2E/two-traders-simple-match/expected.out || echo "Testing matching orders with two traders: failed!\n"
count=$((count+1))


echo "Finished running $count tests!"

# ../spx_exchange E2E/one-seller/products.txt ./trader_sell 
# ../spx_exchange E2E/one-buyer/products.txt ./trader_buy 
