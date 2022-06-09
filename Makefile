CC=gcc
# CFLAGS=-Wall -Werror -Wvla -O0 -g -std=c11 -lm
CFLAGS=-Wall -Werror -Wvla -O0 -g -std=c11 -lm -fsanitize=address,leak
LDFLAGS=-lm
BINARIES=spx_exchange spx_trader

# //  -fsanitize=address,leak

# all: $(BINARIES)

all: clean trader_a trader_b spx_exchange

.PHONY: clean
clean:
	rm -f $(BINARIES)
	rm -f a.out


spx_trader: spx_trader.c
	$(CC) $(CFLAGS) $^ -o $@

spx_exchange: spx_exchange.c
	$(CC) $(CFLAGS) $^ -o spx_exchange
	# clang spx_exchange.c -g -std=c11 -Wall -Werror -o spx_exchange
