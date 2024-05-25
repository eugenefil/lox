## Building and running

Here are the build instructions for Linux.

### Get the source

```
git clone https://github.com/eugenefil/lox
```

### Install dependencies

Use your package manager to install the following packages:

- gcc
- cmake
- make
- ninja
- gtest
- readline

### Build

Underneath, the build system is CMake, but the easiest way to build is
to use the lightweight Makefile wrapper:

```
 make
```

By default, ninja will use all available cores. To limit the number of
parallel build jobs to, say, 2:

```
CMAKE_BUILD_PARALLEL_LEVEL=2 make
```

### Run tests

```
make test
```

### Run the built interpreter

```
./build/lox
```

### Remove build files

```
make clean
```

### Using CMake directly

```sh
 # generate the build system
 cmake -B build -G Ninja

 # build the interpreter
 cmake --build build
 # ...and set the number of parallel jobs
 cmake --build build -j2

 # run the tests
 ctest --test-dir build

 # run the interpreter
 ./build/lox

 # remove build files
 rm -r build
```

## Language grammar

### Statement grammar

```
block ->
      '{' statement* '}'

if-statement ->
      'if' expression block ('else' (block | if-statement))?

parameters ->
      IDENTIFIER (',' IDENTIFIER)*

statement ->
      'print' expression? ';'
    | 'var' IDENTIFIER ('=' expression)? ';'
    | block
    | if-statement
    | 'while' expression block
    | 'for' IDENTIFIER 'in' expression block
    | 'break' ';'
    | 'continue' ';'
    | IDENTIFIER '=' expression ';'
    | 'fn' IDENTIFIER '(' parameters? ')' block
    | return expression? ';'
    | expression ';'

program ->
      statement*
```

### Expression grammar

```
primary ->
      STRING
    | NUMBER
    | 'true'
    | 'false'
    | 'nil'
    | IDENTIFIER
    | '(' expression ')'
    | 'fn' '(' parameters? ')' block

arguments ->
      expression (',' expression)*

call ->
      primary ('(' arguments? ')')*

unary ->
      ('-' | '!') unary
    | call

multiply ->
      unary (('/' | '*' | '%') unary)*

add ->
      multiply (('-' | '+') multiply)*

compare ->
      add (('==' | '!=' | '<' | '>' | '<=' | '>=') add)?

logical_and ->
      compare ('and' compare)*

logical_or ->
      logical_and ('or' logical_and)*

expression ->
      logical_or
```
