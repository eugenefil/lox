## Statement grammar

```
block ->
      '{' statement* '}'

if-statement ->
      'if' expression block ('else' (block | if-statement))?

parameters ->
      IDENTIFIER (',' IDENTIFIER)*

statement ->
      assert expression ';'
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

## Expression grammar

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
