all: build
	@cmake --build build

build:
	@cmake -B build -G Ninja

test:
	@ctest --test-dir build

run:
	@./build/lox

clean:
	@rm -rf build

.PHONY: all test run clean
