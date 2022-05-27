.phony: all configure build clean run-server run-client-listen run-client-connect

ifndef VERBOSE
MAKEFLAGS += --no-print-directory
endif

all: clean configure build

clean:
	rm -rf ./build
	mkdir build

configure:
	/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/gcc -G "Unix Makefiles" -S . -B build/

build:
	/usr/bin/cmake --build ./build/  --target all   -- -j 6

run-server:
	./build/apps/server

run-client-listen:
	./build/apps/client --mode listen --username user_that_listens

run-client-connect:
	./build/apps/client --mode connect --username user_that_connects --client-username user_that_listens

