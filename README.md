Build:

```sh
 make

 # or specify number of parallel jobs to cmake
 CMAKE_BUILD_PARALLEL_LEVEL=2 make
```

Run tests:

```sh
make test
```

Run interpreter:

```sh
 make run

 # or directly
 ./build/lox
```

Remove build files:

```sh
make clean
```

## Statement grammar

```
block ->
      '{' statement* '}'

if-statement ->
      'if' expression block ('else' (block | if-statement))?

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
    | expression ';'

program ->
      statement*
```

## Expression grammar

```
primary ->
      STRING
    | NUMBER
    | IDENTIFIER
    | 'true'
    | 'false'
    | 'nil'
    | '(' expression ')'

unary ->
      ('-' | '!') unary
    | primary

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
