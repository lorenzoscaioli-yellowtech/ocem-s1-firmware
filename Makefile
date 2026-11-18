CC = cc
CPPFLAGS += -Isrc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Werror -pedantic
CORE = src/ccr_ctrl.c src/ccr_steps.c src/ccr_protect.c src/ccr_comm.c src/hal_sim.c
HEADERS = src/ccr.h src/hal.h

.PHONY: all test demo clean
all: build/ccr_sim

build:
	mkdir -p $@

build/ccr_sim: $(CORE) src/main_sim.c $(HEADERS) Makefile | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) src/main_sim.c $(LDFLAGS) -o $@

build/test_ccr: $(CORE) tests/test_ccr.c $(HEADERS) Makefile | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) tests/test_ccr.c $(LDFLAGS) -o $@

test: build/test_ccr
	./build/test_ccr

demo: build/ccr_sim
	./build/ccr_sim

clean:
	rm -f build/ccr_sim build/test_ccr
