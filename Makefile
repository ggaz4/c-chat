.phony: all configure-debug build-debug clean

ifndef VERBOSE
MAKEFLAGS += --no-print-directory
endif

all: clean configure-debug build-debug

clean:
	rm -rf build/*

configure-debug:
	/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/gcc -G "Unix Makefiles" -S . -B build/debug

build-debug:
	/usr/bin/cmake --build ./build/debug  --target all   -j 6
