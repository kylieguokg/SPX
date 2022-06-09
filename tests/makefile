CC=gcc
# CFLAGS=-Wall -Werror -Wvla -O0 -g -std=c11 -lm
CFLAGS=-Wall -Werror -Wvla -O0 -g -std=c11 -lm -fsanitize=address,leak
LDFLAGS=-lm
BINARIES=spx_exchange spx_trader

# //  -fsanitize=address,leak

# all: $(BINARIES)

all: clean trader_sell trader_buy
.PHONY: clean
clean:
	rm -f $(BINARIES)
	rm -f a.out

trader_sell: E2E/one-seller/trader_sell.c
	$(CC) $(CFLAGS) $^ -o $@

trader_buy: E2E/one-buyer/trader_buy.c
	$(CC) $(CFLAGS) $^ -o $@

one-trader-match: E2E/one-trader-match/trader.c
	$(CC) $(CFLAGS) $^ -o $@

two-traders-simple-match_1: E2E/two-traders-simple-match/trader_1.c
	$(CC) $(CFLAGS) $^ -o $@

two-traders-simple-match_2: E2E/two-traders-simple-match/trader_2.c
	$(CC) $(CFLAGS) $^ -o $@

simple-amend: E2E/simple-amend/trader.c
	$(CC) $(CFLAGS) $^ -o $@

simple-cancel: E2E/simple-cancel/trader.c
	$(CC) $(CFLAGS) $^ -o $@
