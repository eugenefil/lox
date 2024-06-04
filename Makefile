all: build
	@cmake --build build

build:
	@cmake -B build -G Ninja

test:
	@ctest --test-dir build -j

clean:
	@rm -rf build

.PHONY: all test clean
