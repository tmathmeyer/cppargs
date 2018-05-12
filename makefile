
GOOGLETEST := googletest/googletest
CC := g++
CFLAGS := --std=c++17

test: test.cpp argparse.h build/ar/googletest.a
	@echo building unit tests
	@mkdir -p build/bin
	$(CC) $(CFLAGS) -I$(GOOGLETEST)/include -lpthread $^ -o build/bin/test

build/ar/googletest.a:
	@echo building google test suite
	@mkdir -p build/obj
	@$(CC) $(CFLAGS) -I$(GOOGLETEST)/include -I$(GOOGLETEST) -c $(GOOGLETEST)/src/gtest-all.cc -o build/obj/gtest-all.o
	@mkdir -p build/ar
	@ar -rv build/ar/googletest.a build/obj/gtest-all.o

clean:
	rm -rf build/
