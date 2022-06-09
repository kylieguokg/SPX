/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../spx_common.h"
#include "../spx_trader.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include "cmocka.h"



// run: gcc test.c stack.c -L. -l cmocka-static
// gcc -fsanitize=address -g -DTESTING -fprofile-arcs -ftest-coverage unit-tests.c spx_exchange.c -lm -L . -l cmocka-static

int write_fd;

static void test_create_trader(){

    assert_null(create_trader(-1));

    trader* new_trader = create_trader(2);

    assert_non_null(new_trader);
    assert_true(new_trader -> active);
    assert_true(new_trader -> order_index);
    assert_non_null(new_trader -> buy_orders);
    assert_non_null(new_trader -> sell_orders);
    assert_non_null(new_trader -> positions);
    assert_non_null(new_trader -> quantities);

    free_trader(new_trader);

}

static void test_get_trader(){

    assert_null(get_trader(0, NULL, 1));

    trader** traders = create_traders(3, 1);
    traders[0] -> pid = 10;
    traders[1] -> pid = 29;
    traders[2] -> pid = 12;

    assert_ptr_equal(get_trader(10, traders, 3), traders[0]);
    assert_ptr_equal(get_trader(29, traders, 3), traders[1]);
    assert_ptr_equal(get_trader(12, traders, 3), traders[2]);

    free_trader(traders[2]);
    free_trader(traders[1]);
    free_trader(traders[0]);
    free(traders);

}

static void test_create_all_orders_array(){

    dyn_array* all_orders = create_all_orders_array(2);

    assert_non_null(all_orders);
    assert_non_null(all_orders -> arr);
    assert_non_null(((order**)(all_orders -> arr))[0]);
    assert_non_null(((order**)(all_orders -> arr))[1]);
    assert_int_equal(all_orders -> size, 2);


    assert_null(create_all_orders_array(-1));

    dyn_array* zero_orders = create_all_orders_array(0);

    assert_non_null(zero_orders);
    assert_int_equal(zero_orders -> size, 0);

    free_all_orders(zero_orders);
    free_all_orders(all_orders);
    
}



static void test_set_trader_positions() {

    set_trader_positions(NULL, 10);

    trader* my_trader = (trader*) malloc(sizeof(trader));

    set_trader_positions(my_trader, 10);

    for (int i = 0; i < 10; i++){
        assert_int_equal((my_trader -> positions)[i], 0);
    }

    free(my_trader -> positions);
    free(my_trader);

}

static void test_set_trader_quantities(){

    set_trader_quantities(NULL, 10);

    trader* my_trader = (trader*) malloc(sizeof(trader));

    set_trader_quantities(my_trader, 10);

    for (int i = 0; i < 10; i++){
        assert_int_equal((my_trader -> quantities)[i], 0);
    }

    free(my_trader -> quantities);
    free(my_trader);

}


static void test_create_dynamic_order_array(){

   dyn_array* out = create_dynamic_order_array();

    assert_int_equal(out -> capacity, DEFAULT_NUM_ORDERS);
    assert_int_equal(out -> size, 0);
    assert_non_null(out -> arr);

    free(out -> arr);
    free(out);

}

static void test_create_order_free_order(){

   order* new_order = create_order();

   assert_non_null(new_order);
   assert_non_null(new_order -> type);
   assert_non_null(new_order -> product);
   assert_non_null(new_order -> trader);

    free_order(new_order);

}

static void test_add_order_null(){

    dyn_array* order_array = create_dynamic_order_array();

    int out = add_order(order_array, NULL);

    assert_int_equal(out, -1);
    assert_int_equal(order_array -> size, 0);
    // assert_null(((order**)(order_array -> arr))[0] -> type);

    free(order_array -> arr);
    free(order_array);

}

static void test_add_order(){

    dyn_array* order_array = create_dynamic_order_array();
    order* new_order = (order*) malloc(sizeof(order));
    new_order -> type = "TEST";

    int out = add_order(order_array, new_order);

    assert_int_equal(out, 0);
    assert_int_equal(order_array -> size, 1);
    assert_ptr_equal(((order**)(order_array -> arr))[0], new_order);
    assert_string_equal((((order**)(order_array -> arr))[0]) -> type, "TEST");

    free(new_order);
    free(order_array -> arr);
    free(order_array);

}

static void test_add_order_sort(){
    
    // second 
    order* sell_order_1 = create_order();
    sell_order_1 -> price = 100;
    sell_order_1 -> quantity = 300;
    strcpy(sell_order_1 -> type, "SELL");
    strcpy(sell_order_1 -> product, "PRODUCT");
    sell_order_1 -> order_id = 0;
    sell_order_1 -> processed_order = 1; // pay fee

    // first 
    order* sell_order_2 = create_order();
    sell_order_2 -> price = 90; // lower price goes first
    sell_order_2 -> quantity = 400;
    strcpy(sell_order_2 -> type, "SELL");
    strcpy(sell_order_2 -> product, "PRODUCT");
    sell_order_2 -> order_id = 0;
    sell_order_2 -> processed_order = 2; // pay fee

    // third
    order* sell_order_3 = create_order();
    sell_order_3 -> price = 100;
    sell_order_3 -> quantity = 300;
    strcpy(sell_order_3 -> type, "SELL");
    strcpy(sell_order_3 -> product, "PRODUCT");
    sell_order_3 -> order_id = 1;
    sell_order_3 -> processed_order = 3; // pay fee

    dyn_array* sell_orders = create_dynamic_order_array();

    add_order(sell_orders, sell_order_1);
    add_order(sell_orders, sell_order_2);
    add_order(sell_orders, sell_order_3);

    assert_ptr_equal(((order**)(sell_orders -> arr))[0], sell_order_2);
    assert_ptr_equal(((order**)(sell_orders -> arr))[1], sell_order_1);
    assert_ptr_equal(((order**)(sell_orders -> arr))[2], sell_order_3);

    free_order_array(sell_orders);
}


void test_cancel_order_info(){

    order* old_order = create_order();
    strcpy(old_order -> type, "BUY");
    strcpy(old_order -> product, "PRODUCT");
    old_order -> price = 28;
    old_order -> quantity = 100;

    order* new_order = create_order();
    strcpy(new_order -> type, "CANCEL");
    strcpy(new_order -> product, "x");
    new_order -> price = 28;
    new_order -> quantity = 100;

    cancel_order_info(NULL, old_order);
    cancel_order_info(NULL, NULL);
    cancel_order_info(new_order, NULL);
    cancel_order_info(new_order, old_order);

    assert_string_equal(new_order -> type, "BUY");
    assert_string_equal(new_order -> product, "PRODUCT");
    assert_int_equal(new_order -> quantity, 0);
    assert_int_equal(new_order -> price, 0);

    free_order(old_order);
    free_order(new_order);

}

void test_amend_order_info(){

    order* old_order = create_order();
    strcpy(old_order -> type, "BUY");
    strcpy(old_order -> product, "PRODUCT");
    old_order -> price = 28;
    old_order -> quantity = 100;

    order* new_order = create_order();
    strcpy(new_order -> type, "AMEND");
    strcpy(new_order -> product, "x");
    new_order -> price = 77;
    new_order -> quantity = 88;

    amend_order_info(NULL, old_order);
    amend_order_info(NULL, NULL);
    amend_order_info(new_order, NULL);
    amend_order_info(new_order, old_order);

    assert_string_equal(new_order -> type, "BUY");
    assert_string_equal(new_order -> product, "PRODUCT");
    assert_int_equal(new_order -> price, 77);
    assert_int_equal(new_order -> quantity, 88);

    free_order(old_order);
    free_order(new_order);

}

void test_check_valid_order_order_id(){

    order* test_order = create_order();
    test_order -> price = 10;
    test_order -> quantity = 10;

    // invalid - must be 0 <= id <= 999999

    test_order -> order_id = -1;
    assert_false(check_valid_order(test_order));

    test_order -> order_id = -123456789;
    assert_false(check_valid_order(test_order));

    test_order -> order_id = 1000000;
    assert_false(check_valid_order(test_order));

    test_order -> order_id = 200000000;
    assert_false(check_valid_order(test_order));

    // valid 
    
    test_order -> order_id = 0;
    assert_true(check_valid_order(test_order));

    test_order -> order_id = 999999;
    assert_true(check_valid_order(test_order));

    test_order -> order_id = 28;
    assert_true(check_valid_order(test_order));

    free_order(test_order);

}

void test_del_order(){

    dyn_array* orders = create_dynamic_order_array();
    order* new_order_1 = create_order();
    order* new_order_2 = create_order();
    new_order_2 -> quantity = 2;
    order* new_order_3 = create_order();

    assert_int_equal(del_order(NULL, 0), -1);
    assert_int_equal(del_order(orders, -1), -1);
    assert_int_equal(del_order(orders, 0), -1);
    assert_int_equal(del_order(orders, 1), -1);

    add_order(orders, new_order_1);
    add_order(orders, new_order_2);
    add_order(orders, new_order_3);

    int out = del_order(orders, 1); // delete new_order_2
    assert_int_equal(out, 0);
    assert_int_equal(orders -> size, 2);
    assert_ptr_equal(((order**)(orders -> arr))[0], new_order_1);
    assert_ptr_equal(((order**)(orders -> arr))[1], new_order_3);
    assert_non_null(new_order_2);
    assert_int_equal(new_order_2 -> quantity, 2);

    assert_int_equal(del_order(orders, 2), -1);

    free_order(new_order_1);
    free_order(new_order_2);
    free_order(new_order_3);
    free(orders -> arr);
    free(orders);


}

void test_remove_order(){

    dyn_array* orders = create_dynamic_order_array();
    order* new_order_1 = create_order();
    order* new_order_2 = create_order();
    order* new_order_3 = create_order();

    assert_int_equal(remove_order(NULL, 0), -1);
    assert_int_equal(remove_order(orders, -1), -1);
    assert_int_equal(remove_order(orders, 0), -1);
    assert_int_equal(remove_order(orders, 1), -1);

    add_order(orders, new_order_1);
    add_order(orders, new_order_2);
    add_order(orders, new_order_3);

    int out = remove_order(orders, 1); // remove new_order_2
    assert_int_equal(out, 0);
    assert_int_equal(orders -> size, 2);
    assert_ptr_equal(((order**)(orders -> arr))[0], new_order_1);
    assert_ptr_equal(((order**)(orders -> arr))[1], new_order_3);

    assert_int_equal(remove_order(orders, 2), -1);

    free_order(new_order_1);
    free_order(new_order_3);
    free(orders -> arr);
    free(orders);


}


void test_check_valid_order_price(){

    order* test_order = create_order();
    test_order -> order_id = 0;
    test_order -> quantity = 10;

    // invalid - must be 1 <= price <= 999999

    test_order -> price = 0;
    assert_false(check_valid_order(test_order));

    test_order -> price = -123456789;
    assert_false(check_valid_order(test_order));

    test_order -> price = 1000000;
    assert_false(check_valid_order(test_order));

    test_order -> price = 200000000;
    assert_false(check_valid_order(test_order));

    // valid 
    
    test_order -> price = 1;
    assert_true(check_valid_order(test_order));

    test_order -> order_id = 999999;
    assert_true(check_valid_order(test_order));

    test_order -> order_id = 28;
    assert_true(check_valid_order(test_order));

    free_order(test_order);

}


void test_check_valid_order_quantity(){

    order* test_order = create_order();
    test_order -> order_id = 0;
    test_order -> price = 10;

    // invalid - must be 1 <= quantity <= 999999

    test_order -> quantity = 0;
    assert_false(check_valid_order(test_order));

    test_order -> quantity = -123456789;
    assert_false(check_valid_order(test_order));

    test_order -> quantity = 1000000;
    assert_false(check_valid_order(test_order));

    test_order -> quantity = 200000000;
    assert_false(check_valid_order(test_order));

    // valid 
    
    test_order -> quantity = 1;
    assert_true(check_valid_order(test_order));

    test_order -> quantity = 999999;
    assert_true(check_valid_order(test_order));

    test_order -> quantity = 28;
    assert_true(check_valid_order(test_order));

    free_order(test_order);

}

void test_get_order(){

    dyn_array* orders = create_dynamic_order_array();
    order* new_order = create_order();
    new_order -> order_id = 0;

    assert_null(get_order(0, NULL));
    assert_null(get_order(-1, orders));
    assert_null(get_order(1000000, orders));

    assert_null(get_order(0, orders));

    add_order(orders, new_order);
    assert_ptr_equal(new_order, get_order(0, orders));

    free_order(new_order);
    
    free(orders -> arr);
    free(orders);

}

void test_check_valid_order_id(){

    dyn_array* orders = create_dynamic_order_array();
    order* order_1 = create_order();
    order_1 -> order_id = 1; 
    trader* trader_1 = create_trader(1);
    trader_1 -> order_index = -1;
    *(order_1 -> trader) = trader_1;
    add_order(trader_1 -> buy_orders, order_1);

    assert_int_equal(check_valid_order_id(NULL, NULL), -2);
    assert_int_equal(check_valid_order_id(order_1, NULL), -2); 
    assert_int_equal(check_valid_order_id(NULL, orders), -2);

    // not incremental - order id is suposed to be 0
    assert_int_equal(check_valid_order_id(order_1, orders), -2); 

    order_1 -> order_id = 0;

    // order not in list
    assert_int_equal(check_valid_order_id(order_1, orders), -1); 

    add_order(orders, order_1);

    // order in list
    assert_int_equal(check_valid_order_id(order_1, orders), 0); 

    trader_1 -> order_index += 1;

    order* order_2 = create_order();
    order_2 -> order_id = 1; 
    *(order_2 -> trader) = trader_1;
    add_order(trader_1 -> buy_orders, order_2);
    add_order(orders, order_2);

    assert_int_equal(check_valid_order_id(order_2, orders), 1); 
    
    free_trader(trader_1);
    free_order(order_1);
    free_order(order_2);
    free(orders -> arr);
    free(orders);

}

void test_get_order_position(){

    dyn_array* orders = create_dynamic_order_array();
    order* new_order = create_order();
    new_order -> order_id = 0;
    *(new_order -> trader) = create_trader(2);
    (*(new_order -> trader)) -> trader_id = 0;

    assert_int_equal(get_order_position(NULL, NULL), -1);
    assert_int_equal(get_order_position(NULL, orders), -1);
    assert_int_equal(get_order_position(new_order, NULL), -1);

    assert_int_equal(get_order_position(new_order, orders), -1);

    add_order(orders, new_order);
    assert_int_equal(get_order_position(new_order, orders), 0);

    order* new_order_2 = create_order();
    new_order_2 -> order_id = 28;
    *(new_order_2 -> trader) = *(new_order -> trader);
    add_order(orders, new_order_2);
    assert_int_equal(get_order_position(new_order_2, orders), 1);

    free_trader(*(new_order -> trader));
    free_order(new_order);
    free_order(new_order_2);
    
    free(orders -> arr);
    free(orders);

}


void test_get_transaction_free(){

    trader* my_trader = create_trader(2);
    long exchange_fees = 0;

    long fee = charge_transaction_fee(100, 100, my_trader, 0, &exchange_fees);

    assert_int_equal(fee, 100);
    assert_int_equal(exchange_fees, 100);
    assert_int_equal((my_trader -> positions)[0], -100);

    fee = charge_transaction_fee(999999, 999999, my_trader, 0, &exchange_fees);

    assert_int_equal(fee, 9999980000);
    assert_int_equal(exchange_fees, 100 + 9999980000 );
    assert_int_equal((my_trader -> positions)[0], -100 - 9999980000);

    fee = charge_transaction_fee(1, 1, my_trader, 0, &exchange_fees);

    assert_int_equal(fee, 0);
    assert_int_equal(exchange_fees, 100 + 9999980000 );
    assert_int_equal((my_trader -> positions)[0], -100 - 9999980000);

    fee = charge_transaction_fee(91, 12, my_trader, 0, &exchange_fees);

    assert_int_equal(fee, 11);
    assert_int_equal(exchange_fees, 100 + 9999980000 + 11 );
    assert_int_equal((my_trader -> positions)[0], -100 - 9999980000 - 11);

    free_trader(my_trader);

}


void test_match_orders_no_orders(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    assert_false(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    order* buy_order = create_order();
    buy_order -> price = 1;
    buy_order -> quantity = 1;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    *(buy_order -> trader) = create_trader(1);
    buy_order -> processed_order = 0; 


    order* sell_order = create_order();
    sell_order -> price = 10;
    sell_order -> quantity = 10;
    strcpy(sell_order -> type, "SELL");
    strcpy(sell_order -> product, "PRODUCT");
    sell_order -> order_id = 0;
    *(sell_order -> trader) = create_trader(1);
    sell_order -> processed_order = 1; 

    assert_false(match_orders(NULL, sell_orders, 0, &exchange_fees));
    assert_false(match_orders(buy_orders, NULL, 0, &exchange_fees));

    // only 1 buy order
    add_order(buy_orders, buy_order);
    assert_false(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    // only 1 sell order
    free_trader(*(buy_order -> trader));
    free_order_array(buy_orders);
    buy_orders = create_dynamic_order_array();
    add_order(sell_orders, sell_order);
    assert_false(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    buy_order = create_order();
    buy_order -> price = 1;
    buy_order -> quantity = 1;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    *(buy_order -> trader) = create_trader(1);
    buy_order -> processed_order = 0; 

    // 1 buy and 1 sell order - do not match
    add_order(buy_orders, buy_order);
    assert_false(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    free_trader(*(buy_order -> trader));
    free_trader(*(sell_order -> trader));
    free_order_array(buy_orders);
    free_order_array(sell_orders);

}

void test_match_orders_simple_exact_match(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    order* buy_order = create_order();
    buy_order -> price = 100;
    buy_order -> quantity = 100;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    trader* buyer = create_trader(1);
    buyer -> write_fd = write_fd;
    buyer -> trader_id = 0;
    *(buy_order -> trader) = buyer;
    buy_order -> processed_order = 0; 

    order* sell_order = create_order();
    sell_order -> price = 100;
    sell_order -> quantity = 100;
    strcpy(sell_order -> type, "SELL");
    strcpy(sell_order -> product, "PRODUCT");
    sell_order -> order_id = 0;
    trader* seller = create_trader(1);
    seller -> write_fd = write_fd;
    seller -> trader_id = 1;
    *(sell_order -> trader) = seller;
    sell_order -> processed_order = 1;  // seller pays fee 

    // only 1 buy order
    add_order(buy_orders, buy_order);
    add_order(sell_orders, sell_order);
    add_order(buyer -> buy_orders, buy_order);
    add_order(seller -> sell_orders, sell_order);

    assert_true(match_orders(buy_orders, sell_orders, 0, &exchange_fees));
    assert_int_equal(exchange_fees, 100);

    // seller paid transaction fee + received 100*100
    assert_int_equal((seller -> positions)[0], 9900);

    // buyer paid 100*100
    assert_int_equal((buyer -> positions)[0], -10000);

    // since both orders were filled, there should be no more orders
    assert_int_equal(sell_orders -> size, 0);
    assert_int_equal(buy_orders -> size, 0);
    assert_int_equal((buyer -> buy_orders) -> size, 0);
    assert_int_equal((seller -> sell_orders) -> size, 0);

    
    free_trader(seller);
    free_trader(buyer);
    free_order_array(buy_orders);
    free_order_array(sell_orders);

}



void test_match_orders_simple_quantity_not_match(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    order* sell_order = create_order();
    sell_order -> price = 100;
    sell_order -> quantity = 100;
    strcpy(sell_order -> type, "SELL");
    strcpy(sell_order -> product, "PRODUCT");
    sell_order -> order_id = 0;
    trader* seller = create_trader(1);
    seller -> write_fd = write_fd;
    seller -> trader_id = 0;
    *(sell_order -> trader) = seller;
    sell_order -> processed_order = 0; 

    order* buy_order = create_order();
    buy_order -> price = 100;
    buy_order -> quantity = 80;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    trader* buyer = create_trader(1);
    buyer -> write_fd = write_fd;
    buyer -> trader_id = 1;
    *(buy_order -> trader) = buyer;
    buy_order -> processed_order = 1; // buyer pays fee

    // only 1 buy order
    add_order(buy_orders, buy_order);
    add_order(sell_orders, sell_order);
    add_order(buyer -> buy_orders, buy_order);
    add_order(seller -> sell_orders, sell_order);

    assert_true(match_orders(buy_orders, sell_orders, 0, &exchange_fees));
    assert_int_equal(exchange_fees, 80);

    // seller received 80*100
    assert_int_equal((seller -> positions)[0], 8000);

    // buyer paid 80*100 and transaction f
    assert_int_equal((buyer -> positions)[0], -8080);

    // seller order not filled completely, buyer order is
    assert_int_equal(sell_orders -> size, 1);
    assert_int_equal(buy_orders -> size, 0);
    assert_int_equal((buyer -> buy_orders) -> size, 0);
    assert_int_equal((seller -> sell_orders) -> size, 1);

    
    free_trader(seller);
    free_trader(buyer);
    free_order_array(buy_orders);
    free_order_array(sell_orders);

}


void test_match_orders_simple_price_not_match(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    order* buy_order = create_order();
    buy_order -> price = 110; // matching price is older order
    buy_order -> quantity = 100;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    trader* buyer = create_trader(1);
    buyer -> write_fd = write_fd;
    buyer -> trader_id = 0;
    *(buy_order -> trader) = buyer;
    buy_order -> processed_order = 0; 

    order* sell_order = create_order();
    sell_order -> price = 90;
    sell_order -> quantity = 100;
    strcpy(sell_order -> type, "SELL");
    strcpy(sell_order -> product, "PRODUCT");
    sell_order -> order_id = 0;
    trader* seller = create_trader(1);
    seller -> write_fd = write_fd;
    seller -> trader_id = 1;
    *(sell_order -> trader) = seller;
    sell_order -> processed_order = 1;  // seller pays fee 

    // only 1 buy order
    add_order(buy_orders, buy_order);
    add_order(sell_orders, sell_order);
    add_order(buyer -> buy_orders, buy_order);
    add_order(seller -> sell_orders, sell_order);

    assert_true(match_orders(buy_orders, sell_orders, 0, &exchange_fees));
    assert_int_equal(exchange_fees, 110);

    // seller paid transaction fee + received 110*100
    assert_int_equal((seller -> positions)[0], 10890);

    // buyer paid 110*100
    assert_int_equal((buyer -> positions)[0], -11000);

    // since both orders were filled, there should be no more orders
    assert_int_equal(sell_orders -> size, 0);
    assert_int_equal(buy_orders -> size, 0);
    assert_int_equal((buyer -> buy_orders) -> size, 0);
    assert_int_equal((seller -> sell_orders) -> size, 0);

    
    free_trader(seller);
    free_trader(buyer);
    free_order_array(buy_orders);
    free_order_array(sell_orders);
}


void test_match_orders_one_buy_multiple_sell_matches(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    order* buy_order = create_order();
    buy_order -> price = 100; // matching price is older order
    buy_order -> quantity = 1000;
    strcpy(buy_order -> type, "BUY");
    strcpy(buy_order -> product, "PRODUCT");
    buy_order -> order_id = 0;
    trader* buyer = create_trader(1);
    buyer -> write_fd = write_fd;
    buyer -> trader_id = 0;
    *(buy_order -> trader) = buyer;
    buy_order -> processed_order = 0; 

    // second 
    order* sell_order_1 = create_order();
    sell_order_1 -> price = 100;
    sell_order_1 -> quantity = 300;
    strcpy(sell_order_1 -> type, "SELL");
    strcpy(sell_order_1 -> product, "PRODUCT");
    sell_order_1 -> order_id = 0;
    trader* seller = create_trader(1);
    seller -> write_fd = write_fd;
    seller -> trader_id = 1;
    *(sell_order_1 -> trader) = seller;
    sell_order_1 -> processed_order = 1; // pay fee

    // first 
    order* sell_order_2 = create_order();
    sell_order_2 -> price = 90; // lower price goes first
    sell_order_2 -> quantity = 400;
    strcpy(sell_order_2 -> type, "SELL");
    strcpy(sell_order_2 -> product, "PRODUCT");
    sell_order_2 -> order_id = 0;
    trader* seller_2 = create_trader(1);
    seller_2 -> write_fd = write_fd;
    seller_2 -> trader_id = 2;
    *(sell_order_2 -> trader) = seller_2;
    sell_order_2 -> processed_order = 2; // pay fee

    // third
    order* sell_order_3 = create_order();
    sell_order_3 -> price = 100;
    sell_order_3 -> quantity = 300;
    strcpy(sell_order_3 -> type, "SELL");
    strcpy(sell_order_3 -> product, "PRODUCT");
    sell_order_3 -> order_id = 1;
    *(sell_order_3 -> trader) = seller;
    sell_order_3 -> processed_order = 3; // pay fee


    // only 1 buy order
    add_order(buy_orders, buy_order);
    add_order(sell_orders, sell_order_1);
    add_order(sell_orders, sell_order_2);
    add_order(sell_orders, sell_order_3);
    add_order(buyer -> buy_orders, buy_order);
    add_order(seller -> sell_orders, sell_order_1);
    add_order(seller -> sell_orders, sell_order_3);
    add_order(seller_2 -> sell_orders, sell_order_2);

    assert_true(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    // 0.01*100(400 + 300 + 300) 
    assert_int_equal(exchange_fees, 1000);

    // seller paid transaction fee + received 100*300 + 100*300
    assert_int_equal((seller -> positions)[0], 59400);

    // seller 2 paid transaction fee + received 100*400
    assert_int_equal((seller_2 -> positions)[0], 39600);

    // buyer paid 100*10000
    assert_int_equal((buyer -> positions)[0], -100000);

    // since all orders were filled, there should be no more orders
    assert_int_equal(sell_orders -> size, 0);
    assert_int_equal(buy_orders -> size, 0);
    assert_int_equal((buyer -> buy_orders) -> size, 0);
    assert_int_equal((seller -> sell_orders) -> size, 0);

    
    free_trader(seller);
    free_trader(seller_2);
    free_trader(buyer);
    free_order_array(buy_orders);
    free_order_array(sell_orders);
}

void test_match_orders_one_seller_multiple_buy_matches(){

    dyn_array* buy_orders = create_dynamic_order_array();
    dyn_array* sell_orders = create_dynamic_order_array();

    long exchange_fees = 0;

    order* sell_order_1 = create_order();
    sell_order_1 -> price = 81;
    sell_order_1 -> quantity = 249;
    strcpy(sell_order_1 -> type, "SELL");
    strcpy(sell_order_1 -> product, "PRODUCT");
    sell_order_1 -> order_id = 0;
    trader* seller = create_trader(1);
    seller -> write_fd = write_fd;
    seller -> trader_id = 1;
    *(sell_order_1 -> trader) = seller;
    sell_order_1 -> processed_order = 1; // pay fee

    // second
    order* buy_order_1 = create_order();
    buy_order_1 -> price = 100; // matching price is older order
    buy_order_1 -> quantity = 91;
    strcpy(buy_order_1 -> type, "BUY");
    strcpy(buy_order_1 -> product, "PRODUCT");
    buy_order_1 -> order_id = 1;
    *(buy_order_1 -> trader) = seller; // same buyer and seller
    buy_order_1 -> processed_order = 0; 

    // first 
    order* buy_order_2 = create_order();
    buy_order_2 -> price = 1000; // matching price is the lower one
    buy_order_2 -> quantity = 111;
    strcpy(buy_order_2 -> type, "BUY");
    strcpy(buy_order_2 -> product, "PRODUCT");
    buy_order_2 -> order_id = 0;
    trader* buyer_2 = create_trader(1);
    buyer_2 -> write_fd = write_fd;
    buyer_2 -> trader_id = 2;
    *(buy_order_2 -> trader) = buyer_2;
    buy_order_2 -> processed_order = 2; // pay fee

    // fourth
    order* buy_order_3 = create_order();
    buy_order_3 -> price = 75; // too low price
    buy_order_3 -> quantity = 300;
    strcpy(buy_order_3 -> type, "BUY");
    strcpy(buy_order_3 -> product, "PRODUCT");
    buy_order_3 -> order_id = 0;
    trader* buyer_3 = create_trader(1);
    buyer_3 -> write_fd = write_fd;
    buyer_3 -> trader_id = 3;
    *(buy_order_3 -> trader) = buyer_3;
    buy_order_3 -> processed_order = 3; // pay fee

    // third - same price as second, but after
    order* buy_order_4 = create_order();
    buy_order_4 -> price = 100; 
    buy_order_4 -> quantity = 49;
    strcpy(buy_order_4 -> type, "BUY");
    strcpy(buy_order_4 -> product, "PRODUCT");
    buy_order_4 -> order_id = 0;
    trader* buyer_4 = create_trader(1);
    buyer_4 -> write_fd = write_fd;
    buyer_4 -> trader_id = 4;
    *(buy_order_4 -> trader) = buyer_4;
    buy_order_4 -> processed_order = 4; // pay fee

    // only 1 sell order
    add_order(sell_orders, sell_order_1);
    add_order(buy_orders, buy_order_1);
    add_order(buy_orders, buy_order_2);
    add_order(buy_orders, buy_order_3);
    add_order(buy_orders, buy_order_4);

    add_order(seller -> sell_orders, sell_order_1);
    add_order(seller -> buy_orders, buy_order_1); 
    add_order(buyer_2 -> buy_orders, buy_order_2); 
    add_order(buyer_3 -> buy_orders, buy_order_3); 
    add_order(buyer_4 -> buy_orders, buy_order_4); 

    assert_true(match_orders(buy_orders, sell_orders, 0, &exchange_fees));

    // 0.01*81(111+ 47) + 0.01*100*(91) = 218.98
    assert_int_equal(exchange_fees, 219);

    // buyer 2 paid transaction fee + paid 81*111 = 9080.91
    assert_int_equal((buyer_2 -> positions)[0], -9081);

    // buyer 3's order was not filled
    assert_int_equal((buyer_3 -> positions)[0], 0);

    // buyer 4 paid transaction fee + paid 81*47 = 3845.07
    assert_int_equal((buyer_4 -> positions)[0], -3845);

    // seller received 81*(111 + 47) + 100*91 and paid 100*91 + transaction fee = 12707
    assert_int_equal((seller -> positions)[0], 12707);

    // buyer 4 order still has q = 2 left, buyer 3 order is unfulfilled
    assert_int_equal(sell_orders -> size, 0);
    assert_int_equal(buy_orders -> size, 2);
    assert_int_equal((seller-> buy_orders) -> size, 0);
    assert_int_equal((seller -> sell_orders) -> size, 0);
    assert_int_equal((buyer_2 -> buy_orders) -> size, 0);
    assert_int_equal((buyer_3 -> buy_orders) -> size, 1);
    assert_int_equal((buyer_4 -> buy_orders) -> size, 1);

    // buy orders should have buy order 4 then 3
    assert_ptr_equal(((order**)(buy_orders -> arr))[0], buy_order_4); 
    assert_ptr_equal(((order**)(buy_orders -> arr))[1], buy_order_3); 


    free_trader(seller);
    free_trader(buyer_2);
    free_trader(buyer_3);
    free_trader(buyer_4);
    free_order_array(buy_orders);
    free_order_array(sell_orders);
}


void test_check_command(){

    // should remove the ;

    char buffer_1[BUFFER_SIZE] = "BUY 2 GPU 1 2;";

    check_command(buffer_1);

    assert_string_equal(buffer_1, "BUY 2 GPU 1 2");


    char buffer_2[BUFFER_SIZE] = "SELL 2 GPU 100 100000000000000;";

    check_command(buffer_2);

    assert_string_equal(buffer_2, "SELL 2 GPU 100 100000000000000");


    char buffer_3[BUFFER_SIZE] = "AMEND 1 1 1;";

    check_command(buffer_3);

    assert_string_equal(buffer_3, "AMEND 1 1 1");


    char buffer_4[BUFFER_SIZE] = "CANCEL 1;";

    check_command(buffer_4);

    assert_string_equal(buffer_4, "CANCEL 1");

    
    char buffer_5[BUFFER_SIZE] = "BUY 1 GPU 0 0;;";

    check_command(buffer_5);

    assert_string_equal(buffer_5, "BUY 1 GPU 0 0");


    char buffer_6[BUFFER_SIZE] = "BUY 1 GPU 0 0;BUY 1 GPU 0 0;";

    check_command(buffer_6);

    assert_string_equal(buffer_6, "BUY 1 GPU 0 0");

    
    char buffer_7[BUFFER_SIZE] = "BUY 1 GP;U 0 0;";

    check_command(buffer_7);

    assert_string_equal(buffer_7, "BUY 1 GP");



}


void test_get_product_orders(){

    assert_null(get_product_orders(NULL, 0));

    dyn_array* all_buy_orders = create_all_orders_array(3);
    dyn_array* all_sell_orders = create_all_orders_array(3);

    assert_non_null(get_product_orders(all_buy_orders, 0));
    assert_non_null(get_product_orders(all_buy_orders, 1));
    assert_non_null(get_product_orders(all_buy_orders, 2));
    assert_null(get_product_orders(all_buy_orders, 3));
    assert_null(get_product_orders(all_buy_orders, -1));

    assert_non_null(get_product_orders(all_sell_orders, 0));
    assert_non_null(get_product_orders(all_sell_orders, 1));
    assert_non_null(get_product_orders(all_sell_orders, 2));
    assert_null(get_product_orders(all_sell_orders, 3));
    assert_null(get_product_orders(all_sell_orders, -1));


    free_all_orders(all_buy_orders);
    free_all_orders(all_sell_orders);

}

void test_add_product(){

    char** products = create_dynamic_product_array(3);

    char* product_1 = "FOOD";

    char* product_2 = "JAM";

    char* product_3 = "MILK";

    add_product(products, product_1, 0);
    add_product(products, product_2, 1);
    add_product(products, product_3, 2);

    assert_string_equal(products[0], "FOOD");
    assert_string_equal(products[1], "JAM");
    assert_string_equal(products[2], "MILK");

    for (int i = 0; i < 3; i++){
        free(products[i]);
    }
    free(products);
}

void test_get_product_position(){

    dyn_array* product_array = malloc(sizeof(dyn_array));
    char** products = create_dynamic_product_array(3);
    add_product(products, "PRODUCT 1", 0);
    add_product(products, "PRODUCT 2", 1);
    add_product(products, "PRODUCT 3", 2);
    product_array -> arr = (void**)products;
    product_array -> size = 3;

    order* new_order = create_order();
    strcpy(new_order -> product, "PRODUCT 1");

    assert_int_equal(get_product_position(NULL, NULL), -1);
    assert_int_equal(get_product_position(product_array, NULL), -1);
    assert_int_equal(get_product_position(NULL, new_order), -1);

    assert_int_equal(get_product_position(product_array, new_order), 0);

    strcpy(new_order -> product, "PRODUCT 2");
    assert_int_equal(get_product_position(product_array, new_order), 1);

    strcpy(new_order -> product, "PRODUCT 3");
    assert_int_equal(get_product_position(product_array, new_order), 2);

    strcpy(new_order -> product, "NON EXISTENT");
    assert_int_equal(get_product_position(product_array, new_order), -1);


    for (int i = 0; i < (product_array -> size); i++){
        free((product_array -> arr)[i]);
    }

    free(products);
    free(product_array);

    free_order(new_order);

}


void test_create_price_level(){

    assert_null(create_price_level(0, 0, NULL));

    price_level* new_price_level = create_price_level(28, 19, "BUY");

    assert_non_null(new_price_level);
    assert_int_equal(new_price_level -> price, 28);
    assert_int_equal(new_price_level -> quantity, 19);
    assert_string_equal(new_price_level -> type, "BUY");
    assert_int_equal(new_price_level -> orders, 1);

    free_price_level(new_price_level);

}

void test_add_price_level(){

    price_level* new_price_level = create_price_level(28, 19, "BUY");
    price_level* new_price_level_2 = create_price_level(90, 19, "BUY");
     price_level* new_price_level_3 = create_price_level(30, 1, "BUY");

    dyn_array* price_array = create_price_level_dyn_array();

    add_price_level(price_array, new_price_level);
    add_price_level(price_array, new_price_level_2);    
    add_price_level(price_array, new_price_level_3);

    assert_ptr_equal(((price_level**)(price_array -> arr))[0], new_price_level);
    assert_ptr_equal(((price_level**)(price_array -> arr))[1], new_price_level_2);
    assert_ptr_equal(((price_level**)(price_array -> arr))[2], new_price_level_3);   

    assert_int_equal(price_array -> size, 3);

    free_price_levels(price_array);

}



void test_get_sell_price_levels(){

    assert_null(get_sell_price_levels(NULL));

    dyn_array* sell_orders = create_dynamic_order_array();

    order* sell_order_1 = create_order();
    sell_order_1 -> price = 100;
    sell_order_1 -> quantity = 100;
    strcpy(sell_order_1 -> type, "SELL");
    strcpy(sell_order_1 -> product, "PRODUCT");
    sell_order_1 -> order_id = 0;
    sell_order_1 -> processed_order = 0;

    order* sell_order_2 = create_order();
    sell_order_2 -> price = 110;
    sell_order_2 -> quantity = 100;
    strcpy(sell_order_2 -> type, "SELL");
    strcpy(sell_order_2 -> product, "PRODUCT");
    sell_order_2 -> order_id = 0;
    sell_order_2 -> processed_order = 1;

    order* sell_order_3 = create_order();
    sell_order_3 -> price = 90;
    sell_order_3 -> quantity = 80;
    strcpy(sell_order_3 -> type, "SELL");
    strcpy(sell_order_3 -> product, "PRODUCT");
    sell_order_3 -> order_id = 0;
    sell_order_3 -> processed_order = 2;

    order* sell_order_4 = create_order();
    sell_order_4 -> price = 100;
    sell_order_4 -> quantity = 200;
    strcpy(sell_order_4 -> type, "SELL");
    strcpy(sell_order_4 -> product, "PRODUCT");
    sell_order_4 -> order_id = 0;
    sell_order_4 -> processed_order = 3;

    order* sell_order_5 = create_order();
    sell_order_5 -> price = 110;
    sell_order_5 -> quantity = 200;
    strcpy(sell_order_5 -> type, "SELL");
    strcpy(sell_order_5 -> product, "PRODUCT");
    sell_order_5 -> order_id = 0;
    sell_order_5 -> processed_order = 4;

    // only 1 buy order
    add_order(sell_orders, sell_order_1);
    add_order(sell_orders, sell_order_2);
    add_order(sell_orders, sell_order_3);
    add_order(sell_orders, sell_order_4);
    add_order(sell_orders, sell_order_5);

    dyn_array* sell_price_levels = get_sell_price_levels(sell_orders);

    assert_int_equal(sell_price_levels -> size, 3); // should be 3 price levels

    price_level** prices = (price_level**)(sell_price_levels -> arr);

    assert_int_equal(prices[0] -> price, 110);
    assert_int_equal(prices[0] -> quantity, 300);
    assert_string_equal(prices[0] -> type, "SELL");
    assert_int_equal(prices[0] -> orders, 2);

    assert_int_equal(prices[1] -> price, 100);
    assert_int_equal(prices[1] -> quantity, 300);
    assert_string_equal(prices[1] -> type, "SELL");
    assert_int_equal(prices[1] -> orders, 2);

    assert_int_equal(prices[2] -> price, 90);
    assert_int_equal(prices[2] -> quantity, 80);
    assert_string_equal(prices[2] -> type, "SELL");
    assert_int_equal(prices[2] -> orders, 1);

    free_order_array(sell_orders);
    free_price_levels(sell_price_levels);


}

void test_get_buy_price_levels(){

    assert_null(get_buy_price_levels(NULL));

    dyn_array* buy_orders = create_dynamic_order_array();

    order* buy_order_1 = create_order();
    buy_order_1 -> price = 100;
    buy_order_1 -> quantity = 100;
    strcpy(buy_order_1 -> type, "BUY");
    strcpy(buy_order_1 -> product, "PRODUCT");
    buy_order_1 -> order_id = 0;
    buy_order_1 -> processed_order = 0;

    order* buy_order_2 = create_order();
    buy_order_2 -> price = 110;
    buy_order_2 -> quantity = 100;
    strcpy(buy_order_2 -> type, "BUY");
    strcpy(buy_order_2 -> product, "PRODUCT");
    buy_order_2 -> order_id = 0;
    buy_order_2 -> processed_order = 1;


    order* buy_order_3 = create_order();
    buy_order_3 -> price = 90;
    buy_order_3 -> quantity = 80;
    strcpy(buy_order_3 -> type, "BUY");
    strcpy(buy_order_3 -> product, "PRODUCT");
    buy_order_3 -> order_id = 0;
    buy_order_3 -> processed_order = 2;

    order* buy_order_4 = create_order();
    buy_order_4 -> price = 100;
    buy_order_4 -> quantity = 200;
    strcpy(buy_order_4 -> type, "BUY");
    strcpy(buy_order_4 -> product, "PRODUCT");
    buy_order_4 -> order_id = 0;
    buy_order_4 -> processed_order = 3;

    order* buy_order_5 = create_order();
    buy_order_5 -> price = 110;
    buy_order_5 -> quantity = 200;
    strcpy(buy_order_5 -> type, "BUY");
    strcpy(buy_order_5 -> product, "PRODUCT");
    buy_order_5 -> order_id = 0;
    buy_order_5 -> processed_order = 4;

    // only 1 buy order
    add_order(buy_orders, buy_order_1);
    add_order(buy_orders, buy_order_2);
    add_order(buy_orders, buy_order_3);
    add_order(buy_orders, buy_order_4);
    add_order(buy_orders, buy_order_5);

    dyn_array* buy_price_levels = get_buy_price_levels(buy_orders);

    assert_int_equal(buy_price_levels -> size, 3); // should be 3 price levels

    price_level** prices = (price_level**)(buy_price_levels -> arr);

    assert_int_equal(prices[0] -> price, 110);
    assert_int_equal(prices[0] -> quantity, 300);
    assert_string_equal(prices[0] -> type, "BUY");
    assert_int_equal(prices[0] -> orders, 2);

    assert_int_equal(prices[1] -> price, 100);
    assert_int_equal(prices[1] -> quantity, 300);
    assert_string_equal(prices[1] -> type, "BUY");
    assert_int_equal(prices[1] -> orders, 2);

    assert_int_equal(prices[2] -> price, 90);
    assert_int_equal(prices[2] -> quantity, 80);
    assert_string_equal(prices[2] -> type, "BUY");
    assert_int_equal(prices[2] -> orders, 1);

    free_order_array(buy_orders);
    free_price_levels(buy_price_levels);


}


static void test_buy_price_time_position(){

    // price highest to lowest

    order* buy_order_1 = create_order();
    buy_order_1 -> price = 100;
    buy_order_1 -> quantity = 100;
    strcpy(buy_order_1 -> type, "BUY");
    strcpy(buy_order_1 -> product, "PRODUCT");
    buy_order_1 -> order_id = 0;
    buy_order_1 -> processed_order = 0;

    order* buy_order_2 = create_order();
    buy_order_2 -> price = 100;
    buy_order_2 -> quantity = 100;
    strcpy(buy_order_2 -> type, "BUY");
    strcpy(buy_order_2 -> product, "PRODUCT");
    buy_order_2 -> order_id = 0;
    buy_order_2 -> processed_order = 1;

    // same price, order_1 goes first
    assert_int_equal(buy_price_time_position((const void*)&buy_order_1, (const void*)&buy_order_2), -1);

    buy_order_2 -> price = 1000;
    // order 2 has highest price, goes first
    assert_int_equal(buy_price_time_position((const void*)&buy_order_1, (const void*)&buy_order_2), 1);

    buy_order_1 -> price = 1200;
    // order 1 has highest price, goes first
    assert_int_equal(buy_price_time_position((const void*)&buy_order_1, (const void*)&buy_order_2), -1);

    free_order(buy_order_1);
    free_order(buy_order_2);

}


static void test_sell_price_time_position(){

    // price lowest to highest

    order* sell_order_1 = create_order();
    sell_order_1 -> price = 100;
    sell_order_1 -> quantity = 100;
    strcpy(sell_order_1 -> type, "sell");
    strcpy(sell_order_1 -> product, "PRODUCT");
    sell_order_1 -> order_id = 0;
    sell_order_1 -> processed_order = 0;

    order* sell_order_2 = create_order();
    sell_order_2 -> price = 100;
    sell_order_2 -> quantity = 100;
    strcpy(sell_order_2 -> type, "sell");
    strcpy(sell_order_2 -> product, "PRODUCT");
    sell_order_2 -> order_id = 0;
    sell_order_2 -> processed_order = 1;

    // same price, order_1 goes first
    assert_int_equal(sell_price_time_position((const void*)&sell_order_1, (const void*)&sell_order_2), -1);

    sell_order_2 -> price = 90;
    // order 2 has lower price, goes first
    assert_int_equal(sell_price_time_position((const void*)&sell_order_1, (const void*)&sell_order_2), 1);

    sell_order_1 -> price = 20;
    // order 1 has lowest price, goes first
    assert_int_equal(sell_price_time_position((const void*)&sell_order_1, (const void*)&sell_order_2), -1);

    free_order(sell_order_1);
    free_order(sell_order_2);



}




int main(void) {

    // close stdout
    // int pipe_fd[2] = {-1, -1};
	// pipe(pipe_fd);
    // dup2(pipe_fd[1], STDOUT_FILENO);
    // write_fd = pipe_fd[1];

    const struct CMUnitTest tests[] = {
        
        cmocka_unit_test(test_get_trader),
        cmocka_unit_test(test_set_trader_positions),
        cmocka_unit_test(test_set_trader_quantities),
        cmocka_unit_test(test_create_trader),

        cmocka_unit_test(test_create_dynamic_order_array),
        cmocka_unit_test(test_create_all_orders_array),
        cmocka_unit_test(test_add_order_null),
        cmocka_unit_test(test_add_order),
        cmocka_unit_test(test_add_order_sort),
        cmocka_unit_test(test_cancel_order_info),
        cmocka_unit_test(test_amend_order_info),
        cmocka_unit_test(test_check_valid_order_order_id),
        cmocka_unit_test(test_check_valid_order_price),
        cmocka_unit_test(test_check_valid_order_quantity),
        cmocka_unit_test(test_get_order),
        cmocka_unit_test(test_get_order_position),
        cmocka_unit_test(test_create_order_free_order),
        cmocka_unit_test(test_del_order),
        cmocka_unit_test(test_remove_order),
        cmocka_unit_test(test_get_transaction_free),
        cmocka_unit_test(test_match_orders_no_orders),
        cmocka_unit_test(test_match_orders_simple_exact_match),
        cmocka_unit_test(test_match_orders_simple_quantity_not_match),
        cmocka_unit_test(test_match_orders_simple_price_not_match),
        cmocka_unit_test(test_match_orders_one_buy_multiple_sell_matches),
        cmocka_unit_test(test_match_orders_one_seller_multiple_buy_matches),
        cmocka_unit_test(test_check_valid_order_id),
        cmocka_unit_test(test_check_command),

        cmocka_unit_test(test_get_product_orders),
        cmocka_unit_test(test_get_product_position),
        cmocka_unit_test(test_add_product),

        cmocka_unit_test(test_get_sell_price_levels),
        cmocka_unit_test(test_get_buy_price_levels),
        cmocka_unit_test(test_create_price_level),
        cmocka_unit_test(test_add_price_level),
        cmocka_unit_test(test_buy_price_time_position),
        cmocka_unit_test(test_sell_price_time_position)
    

     };



    return cmocka_run_group_tests(tests, NULL, NULL);
}
