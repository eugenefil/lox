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

expression ->
      compare
```
